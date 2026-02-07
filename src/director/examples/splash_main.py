from pathlib import Path

from director.splash_screen import SplashScreen

splash = SplashScreen(
    title="Director",
    splash_settings_name="Director/DirectorSplashScreen",
    app_name="Director_MainWindowApp",
    image_path=Path(__file__).resolve().parent.parent / "assets" / "python_logo.png",
    show_module_list=False,  # optional arg to show module names as they are imported
)

splash.begin_imports()
from director import mainwindowapp

splash.begin_construction()
fields = mainwindowapp.construct()

splash.begin_start()
fields.app.start()
