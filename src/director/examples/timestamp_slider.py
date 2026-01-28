from director import mainwindowapp, timestamp_slider
from director import visualization as vis


def on_time_changed(timestamp_s: float):
    print(f"Timestamp: {timestamp_s}")
    vis.updateText(f"Timestamp: {timestamp_s:.2f}", "timestamp")


if __name__ == "__main__":
    fields = mainwindowapp.construct()

    slider = timestamp_slider.TimestampSlider()
    slider.add_to_toolbar(fields.app, "Timestamp Slider")
    slider.add_keyboard_shortcuts(fields.mainWindow)
    slider.slider.initMouseScrubber(fields.mainWindow)
    slider.connect_on_time_changed(on_time_changed)
    slider.set_time_range(0.0, 10.0)
    slider.set_time(0.0)

    fields.app.start()
