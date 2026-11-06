package org.opendmfbmx.app;

import org.libsdl.app.SDLActivity;

/*
 * SDL's activity with our libraries and command line (docs/PORTING.md "X6 step 1 (Android arm64)"). SDL loads
 * libSDL3.so and libmain.so and calls libmain.so's SDL_main (src/native/main.c) on its own thread; that changes
 * into the game folder under the app's external files folder (src/native/android.c).
 */
public class BmxActivity extends SDLActivity {
    @Override
    protected String[] getLibraries() {
        return new String[] {"SDL3", "main"};
    }

    @Override
    protected String[] getArguments() {
        return new String[] {"Scene.Txt"};
    }
}
