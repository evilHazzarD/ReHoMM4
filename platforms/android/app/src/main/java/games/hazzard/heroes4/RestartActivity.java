package games.hazzard.heroes4;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.os.Process;

// Relaunches the game in a fresh process (a new language: the game reads its text and fonts once,
// at start). It runs in its own process (":restart", AndroidManifest.xml): ends the game's, starts
// the game again and ends itself.
public class RestartActivity extends Activity {
    @Override
    protected void onCreate(Bundle b) {
        super.onCreate(b);
        Process.killProcess(getIntent().getIntExtra("pid", -1));
        Intent i = new Intent(this, HeroesActivity.class);
        i.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK | Intent.FLAG_ACTIVITY_CLEAR_TASK);
        startActivity(i);
        finish();
        Process.killProcess(Process.myPid());
    }
}
