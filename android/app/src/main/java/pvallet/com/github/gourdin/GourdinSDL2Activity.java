package pvallet.com.github.gourdin;

import org.libsdl.app.SDLActivity;
import android.view.View;
import android.util.DisplayMetrics;

public class GourdinSDL2Activity extends SDLActivity
{
	 /**
     * This method is called by SDL before loading the native shared libraries.
     * It can be overridden to provide names of shared libraries to be loaded.
     * The default implementation returns the defaults. It never returns null.
     * An array returned by a new implementation must at least contain "SDL2".
     * Also keep in mind that the order the libraries are loaded may matter.
     *
     * @return names of shared libraries to be loaded (e.g. "SDL2", "main").
     */
    @Override
    protected String[] getLibraries() {
        return new String[]{
                "SDL2",
                "SDL2_image",
                "gourdin"
        };
    }

    @Override
    protected void onResume() {
        super.onResume();

        // Trigger immersive mode
        View decorView = getWindow().getDecorView();
        decorView.setSystemUiVisibility(View.SYSTEM_UI_FLAG_LAYOUT_STABLE
                                      | View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
                                      | View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
                                      | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
                                      | View.SYSTEM_UI_FLAG_FULLSCREEN
                                      | View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY);
    }

    float getXDPI() {
      DisplayMetrics metrics = getResources().getDisplayMetrics();

      return metrics.xdpi;
    }

    @Override
    public void setOrientationBis(int w, int h, boolean resizable, String hint) {
      // Forcing gourdin to be landscape
      super.setOrientationBis(w,h,resizable, "LandscapeRight LandscapeLeft");
    }
}
