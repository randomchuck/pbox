#if defined(UNICODE) && !defined(_UNICODE)
    #define _UNICODE
#elif defined(_UNICODE) && !defined(UNICODE)
    #define UNICODE
#endif

#include <tchar.h>
#include <windows.h>
#include <stdio.h>

// Physics Box.
#include "PBox.h"
// Soft3DRenderer
#include "../Boop/Boop3D.h"

HWND hwnd;               /* This is the handle for our window */
Boop3D boop;

//const int winwidth = 544;
//const int winheight = 375;
const int winwidth = 1000;
const int winheight = 800;

/*  Declare Windows procedure  */
LRESULT CALLBACK WindowProcedure (HWND, UINT, WPARAM, LPARAM);

/*  Make the class name into a global variable  */
TCHAR szClassName[ ] = _T("CodeBlocksWindowsApp");

///////////////////////////////////////////////////////////////////////////////
//
int WINAPI WinMain (HINSTANCE hThisInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR lpszArgument,
                     int nCmdShow)
{
    MSG messages;            /* Here messages to the application are saved */
    WNDCLASSEX wincl;        /* Data structure for the windowclass */

    /* The Window structure */
    wincl.hInstance = hThisInstance;
    wincl.lpszClassName = szClassName;
    wincl.lpfnWndProc = WindowProcedure;      /* This function is called by windows */
    wincl.style = CS_DBLCLKS;                 /* Catch double-clicks */
    wincl.cbSize = sizeof (WNDCLASSEX);

    /* Use default icon and mouse-pointer */
    wincl.hIcon = LoadIcon (NULL, IDI_APPLICATION);
    wincl.hIconSm = LoadIcon (NULL, IDI_APPLICATION);
    wincl.hCursor = LoadCursor (NULL, IDC_ARROW);
    wincl.lpszMenuName = NULL;                 /* No menu */
    wincl.cbClsExtra = 0;                      /* No extra bytes after the window class */
    wincl.cbWndExtra = 0;                      /* structure or the window instance */
    /* Use Windows's default colour as the background of the window */
    wincl.hbrBackground = (HBRUSH) COLOR_BACKGROUND;

    /* Register the window class, and if it fails quit the program */
    if (!RegisterClassEx (&wincl))
        return 0;

    /* The class is registered, let's create the program*/
    hwnd = CreateWindowEx (
           0,                   /* Extended possibilites for variation */
           szClassName,         /* Classname */
           _T("Code::Blocks Template Windows App"),       /* Title Text */
           WS_OVERLAPPEDWINDOW, /* default window */
           CW_USEDEFAULT,       /* Windows decides the position */
           CW_USEDEFAULT,       /* where the window ends up on the screen */
           winwidth,                 /* The programs width */
           winheight,                 /* and height in pixels */
           HWND_DESKTOP,        /* The window is a child-window to desktop */
           NULL,                /* No menu */
           hThisInstance,       /* Program Instance handler */
           NULL                 /* No Window Creation data */
           );

    /* Make the window visible on the screen */
    ShowWindow (hwnd, nCmdShow);

	///////////////
	// Physics Box.

		vec3 vpos ( 0, 1, 0 );
		vec3 vsize( 1, 1, 1 );
		vec3 vscal( 1, 1, 1 );
		vec3 vrota( 0, 0, 1 );
		float vang = 0.0f;
		const int numboxes = 10;
		PBox pboxes[numboxes];
		for( int bx = 0; bx < numboxes - 1; bx++ ) {
			pboxes[bx] = PBox( vpos + vec3((bx % 2) * 0.5f,  1 + bx * 1.25f, 0), vsize, vscal, vrota, vang, true );
			pboxes[bx].setvel( vec3(0, -0.01f, 0) );
		}
		pboxes[numboxes - 1] = PBox( vec3(0, 0, 0), vec3(1, 1, 1), vec3(4, 1, 4), vrota, vang, false );

	// Physics Box.
	///////////////

	// 3D Renderer.
	boop.Initialize(hwnd);
	for( int m = 0; m < numboxes; m++ ) {
		boop.LoadMesh( "box.obj", "roadtile.bmp" );
		// boop.GetMesh(m)->matrix *= scale( vec3(2.0f, 2.0f, 2.0f) );
	}
	boop.CameraLookat( vec3(1, 4.0f, 2.75f), vec3(0, 2, 0), vec3(0, 1, 0) );
	boop.SetShading(2);

	// Message Loop runs indefinitely until user presses
	// X Button or ESC Key.
	while( true ) {

		if( GetAsyncKeyState(VK_SPACE) & 0x8000 ) {
			for( int bx = 0; bx < numboxes - 1; bx++ ) {
				pboxes[bx] = PBox( vpos + vec3((bx % 2) * 0.5f,  1 + bx * 1.25f, 0), vsize, vscal, vrota, vang, true );
				pboxes[bx].setvel( vec3(0, -0.01f, 0) );
			}
		}

		if(true)
		{
			static int stimer = 0;
			static int numcnts = 1;
			static int avgtime = 0;
			if( stimer == 0 )
				stimer = GetTickCount();

			// Update box velocities, position, etc.
			for(int u = 0; u < 5; u++)
				PBox::update( pboxes, numboxes );

			char strbfr[100] = {0};
			avgtime += (GetTickCount() - stimer);
			sprintf( strbfr, "UpdateBoxes() Time - %d", GetTickCount() - stimer );
			TextOut( boop.GetBackbuffer(), 10, 85, strbfr, strlen(strbfr) );
			for( int c = 0; c < 100; c++ ) strbfr[c] = 0;

			sprintf( strbfr, "UpdateBoxes() Avg Time - %d", avgtime / numcnts );
			TextOut( boop.GetBackbuffer(), 10, 100, strbfr, strlen(strbfr) );
			stimer = 0;
			numcnts++;
		}

		for( int cb = 0; cb < numboxes; cb++ ) {
			boop.GetMesh(cb)->matrix = pboxes[cb].mat;
			boop.GetMesh(cb)->matrix *= scale( vec3(0.5f, 0.5f, 0.5f) );
		}

		// Render 3D.
		boop.Render();

		// Wireframe, No Shading, Flat Shading, Gouraud Shading,
		// Texture, No Texture.
		// Shading and Texturing can be mixed.
		// Performance increases when using No Textures and
		// No Shading.
		if( GetAsyncKeyState(0x31) & 0x8000 )
			boop.SetShading(0);
		if( GetAsyncKeyState(0x32) & 0x8000 )
			boop.SetShading(1);
		if( GetAsyncKeyState(0x33) & 0x8000 )
			boop.SetShading(2);
		if( GetAsyncKeyState(0x34) & 0x8000 )
			boop.SetShading(3);
		if( GetAsyncKeyState(0x35) & 0x8000 )
			boop.SetTextures(0);
		if( GetAsyncKeyState(0x36) & 0x8000 )
			boop.SetTextures(1);
		if( GetAsyncKeyState(0x37) & 0x8000 )
			boop.SetTextures(2);

		///////////////////
		// Camera Controls

			// Move camera left, right, forward, back, up, down.
			float spd = 0.1f;
			// x.
			if( GetAsyncKeyState(VK_LEFT) & 0x8000 ) {
				boop.CameraStrafeToA( vec3(-spd, 0, 0) );
			}
			if( GetAsyncKeyState(VK_RIGHT) & 0x8000 ) {
				boop.CameraStrafeToA( vec3(spd, 0, 0) );
			}
			// y.
			if( GetAsyncKeyState(VK_NEXT) & 0x8000 ) {
				boop.CameraStrafeToA( vec3(0, -spd, 0) );
			}
			if( GetAsyncKeyState(VK_PRIOR) & 0x8000 ) {
				boop.CameraStrafeToA( vec3(0, spd, 0) );
			}
			// z.
			if( GetAsyncKeyState(VK_UP) & 0x8000 ) {
				boop.CameraStrafeToA( vec3(0, 0, -spd) );
			}
			if( GetAsyncKeyState(VK_DOWN) & 0x8000 ) {
				boop.CameraStrafeToA( vec3(0, 0, spd) );
			}

		// Camera Controls
		///////////////////

		// Grab windows messages.
        if( PeekMessage(&messages, NULL, 0, 0, PM_REMOVE) ) {

			if( messages.message == WM_LBUTTONUP ) {
				int chicken = 0;
			}

			if( messages.message == WM_QUIT || messages.message == WM_CLOSE )
				break;

			if( GetAsyncKeyState(VK_ESCAPE) & 0x8000 )
				PostQuitMessage(0);

			/* Translate virtual-key messages into character messages */
			TranslateMessage(&messages);
			/* Send message to WindowProcedure */
			DispatchMessage(&messages);
		}
		// So we don't kill the cpu.
		// Roughly 60 iterations a second.
		Sleep(16);
	}

	// Clean up.
	boop.Shutdown();

    /* The program return-value is 0 - The value that PostQuitMessage() gave */
    return messages.wParam;
}


/*  This function is called by the Windows function DispatchMessage()  */

LRESULT CALLBACK WindowProcedure (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)                  /* handle the messages */
    {
        case WM_DESTROY: {
            PostQuitMessage (0);       /* send a WM_QUIT to the message queue */
            break;
		}
		case WM_SIZE: {
			int param1 = LOWORD(wParam);
			int param2 = HIWORD(wParam);
			boop.ResizeView(param1, param2);
		}
        default:                      /* for messages that we don't deal with */
            return DefWindowProc (hwnd, message, wParam, lParam);
    }

    return DefWindowProc (hwnd, message, wParam, lParam);
}
