#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>

#include "Application.h"
#include "Renderer.h"
#include <ospray/ospray.h>

#include "Debug.h"

using namespace std;

#include "async.h"
#include "quat.h"
#include "async-server.h"
#include "Socket.h"

Socket *skt;

KEYED_OBJECT_TYPE(ServerRendering)

void
ServerRendering::initialize()
{
  Rendering::initialize();
}

pthread_mutex_t alp_lock = PTHREAD_MUTEX_INITIALIZER;

int fknt[1000];
int max_f = -1;
bool render_one = false;


void
ServerRendering::AddLocalPixels(Pixel *p, int n, int f)
{
	if (f > max_f) max_f = f;
	{
		for (int i = max_f + 1; i <= f; i++)
			fknt[i] = 0;
		max_f = f;
	}
	fknt[f] += n;

	if (f == GetFrame())
	{
		char* ptrs[] = {(char *)&n, (char *)&f, (char *)p};
		int   szs[] = {sizeof(int), sizeof(int), static_cast<int>(n*sizeof(Pixel)), 0};

		pthread_mutex_lock(&alp_lock);
		skt->SendV(ptrs, szs);
		pthread_mutex_unlock(&alp_lock);
	}

	Rendering::AddLocalPixels(p, n, f);
}

float X0, Y0;
float X1, Y1;
float *buf = NULL;
bool quit = false;

void *
render_thread(void *buf)
{
	int width = ((int *)buf)[1];
	int height = ((int *)buf)[2];
	string statefile = string(((char *)buf) + 3*sizeof(int));

  RendererP theRenderer = Renderer::NewP();
  Document *doc = GetTheApplication()->OpenInputState(statefile);
  theRenderer->LoadStateFromDocument(*doc);

	free(buf);

  vector<CameraP> theCameras = Camera::LoadCamerasFromJSON(*doc);
  CameraP theCamera = theCameras[0];

  vec3f scaled_viewdirection, center, viewpoint, viewdirection, viewup;
  theCamera->get_viewpoint(viewpoint);
  theCamera->get_viewdirection(viewdirection);
  theCamera->get_viewup(viewup);
  add(viewpoint, viewdirection, center);
	
  float aov, viewdistance = len(viewdirection);
	theCamera->get_angle_of_view(aov);

  normalize(viewdirection);
  normalize(viewup);

	scaled_viewdirection = viewdirection;
	scale(viewdistance, scaled_viewdirection);

  vec3f viewright;
  cross(viewdirection, viewup, viewright);
  cross(viewright, viewdirection, viewup);

  vec3f y(0.0, 1.0, 0.0);
  float ay = acos(dot(y, viewup));

  vec4f current_rotation;
  axis_to_quat(viewdirection, ay, current_rotation);

  DatasetsP theDatasets = Datasets::NewP();
  theDatasets->LoadFromJSON(*doc);
  theDatasets->Commit();

  vector<VisualizationP> theVisualizations = Visualization::LoadVisualizationsFromJSON(*doc);
  VisualizationP v = theVisualizations[0];
	v->Commit(theDatasets);

  RenderingSetP theRenderingSet = RenderingSet::NewP();
  RenderingP    theRendering = ServerRendering::NewP();
  theRendering->SetTheOwner(0);
  theRendering->SetTheSize(width, height);
  theRendering->SetTheDatasets(theDatasets);
  theRendering->SetTheVisualization(v);
  theRendering->SetTheRenderingSet(theRenderingSet);
	theRendering->SetTheCamera(theCamera);
  theRendering->Commit();
  theRenderingSet->AddRendering(theRendering);
  theRenderingSet->Commit();

	theCamera->Commit();

	int frame = 0;
	cerr << "---------------- " << frame++ << "\n";
	cerr << "dir:  " << viewdirection.x << " " << viewdirection.y << " " << viewdirection.z << "\n";
	cerr << "sdir: " << scaled_viewdirection.x << " " << scaled_viewdirection.y << " " << scaled_viewdirection.z << "\n";
	cerr << "vp:   " << viewpoint.x << " " << viewpoint.y << " " << viewpoint.z << "\n";
	cerr << "up:   " << viewup.x << " " << viewup.y << " " << viewup.z << "\n";
	cerr << "aov:  " << aov << "\n";
	cerr << "vdist:" << viewdistance << "\n";

	theRenderer->Render(theRenderingSet);

	while (! quit)
	{
		float x1 = X1, y1 = Y1;   // So stays unclanged by other thread during rendering

		float dx = (x1 - X0);
		float dy = (y1 - Y0);

		bool cam_moved = sqrt(dx*dx + dy*dy) > 0.001;
		if (render_one || cam_moved)
		{
			render_one = false;

			if (cam_moved)
			{
				vec4f this_rotation;
				trackball(this_rotation, X0, Y0, x1, y1);

				vec4f next_rotation;
				add_quats(this_rotation, current_rotation, next_rotation);
				current_rotation = next_rotation;

				vec3f y(0.0, 1.0, 0.0);
				vec3f z(0.0, 0.0, 1.0);

				rotate_vector_by_quat(y, current_rotation, viewup);
				rotate_vector_by_quat(z, current_rotation, viewdirection);

				// scaled_viewdirection.x = viewdirection.x * viewdistance;
				// scaled_viewdirection.y = viewdirection.y * viewdistance;
				// scaled_viewdirection.z = viewdirection.z * viewdistance;

				scaled_viewdirection = viewdirection;
				scale(viewdistance, scaled_viewdirection);

				sub(center, scaled_viewdirection, viewpoint);
				X0 = x1; Y0 = y1;
			}

			cerr << "---------------- " << frame++ << "\n";
			cerr << "dir:  " << viewdirection.x << " " << viewdirection.y << " " << viewdirection.z << "\n";
			cerr << "sdir: " << scaled_viewdirection.x << " " << scaled_viewdirection.y << " " << scaled_viewdirection.z << "\n";
			cerr << "vp:   " << viewpoint.x << " " << viewpoint.y << " " << viewpoint.z << "\n";
			cerr << "up:   " << viewup.x << " " << viewup.y << " " << viewup.z << "\n";
			cerr << "aov:  " << aov << "\n";
			cerr << "vdist:" << viewdistance << "\n";

			CameraP newCamera = Camera::NewP();
			newCamera->set_viewdirection(scaled_viewdirection); 
			newCamera->set_viewpoint(viewpoint);
			newCamera->set_viewup(viewup);
			newCamera->set_angle_of_view(aov);
			newCamera->Commit();

			RenderingSetP newRenderingSet = RenderingSet::NewP();
			RenderingP    newRendering = ServerRendering::NewP();
			newRendering->SetTheOwner(0);
			newRendering->SetTheSize(width, height);
			newRendering->SetTheDatasets(theDatasets);
			newRendering->SetTheVisualization(v);
			newRendering->SetTheRenderingSet(newRenderingSet);
			newRendering->SetTheCamera(newCamera);
			newRendering->Commit();
			newRenderingSet->AddRendering(newRendering);
			newRenderingSet->Commit();

			theRenderer->Render(newRenderingSet);
		}
	}
}

void
syntax(char *a)
{
  cerr << "syntax: " << a << " [options]\n";
  cerr << "options:\n";
  cerr << "  -D         run debugger\n";
  cerr << "  -A         wait for attachment\n";
  cerr << "  -P port    port to use (5001)\n";
  exit(1);
}

int
main(int argc, char *argv[])
{
  bool dbg = false, atch = false;
	int port = 5001;

  ospInit(&argc, (const char **)argv);

  Application theApplication(&argc, &argv);
  theApplication.Start();

	ServerRendering::RegisterClass();

  for (int i = 1; i < argc; i++)
  {
    if (!strcmp(argv[i], "-A")) dbg = true, atch = true;
    else if (!strcmp(argv[i], "-D")) dbg = true, atch = false;
    else if (!strcmp(argv[i], "-P")) port = atoi(argv[++i]);
    else
      syntax(argv[0]);
  }

  Debug *d = dbg ? new Debug(argv[0], atch) : NULL;

  Renderer::Initialize();
  GetTheApplication()->Run();

  if (GetTheApplication()->GetRank() == 0)
	{
		skt = new Socket(port);

		pthread_t render_tid = 0;

		while (!quit)
		{
			char *buf; int n;
			skt->Recv(buf, n);

			int op = *(int *)buf;
			void *args = (void *)(buf + sizeof(int));

			switch(*(int *)buf)
			{
				case RENDER_ONE:
					render_one = true;
					break;

				case DEBUG:
					for (int i = 0; i <= max_f; i++)
						std::cerr << i << ": " << fknt[i] << "\n";
					break;

				case QUIT:
					quit = true;
					free(buf);
					pthread_join(render_tid, NULL);
					break;
		
				case START:
					if (render_tid != 0)
					{
						std::cerr << "rendering thread already running!\n";
						exit(0);
					}
					pthread_create(&render_tid, NULL, render_thread, buf); // buf, so lower level can delete
					break;
				
				case MOUSEDOWN:
					X0 = X1 = ((float *)args)[0];
					Y0 = Y1 = ((float *)args)[1];
					free(buf);
					break;

				case MOUSEMOTION:
					X1 = ((float *)args)[0];
					Y1 = ((float *)args)[1];
					free(buf);
					break;
			}
		}
	}
  else
    GetTheApplication()->Wait();

  return 0;
}
