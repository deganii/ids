import sys, os
sys.path.append(os.path.join(os.path.dirname(__file__), '.'))
os.environ['KIVY_GL_BACKEND'] = 'gl'
os.environ['KIVY_BCM_DISPMANX_OVERLAY'] = "1"
os.environ['KIVY_BCM_DISPMANX_LAYER'] = "1"

from kivy.app import App
from kivy.clock import Clock
from kivy.lang import Builder
# from camera.camera_ids import CameraIDS
from camera.histogram import Histogram
from camera.image_button import ImageButton
from camera.tracker_histogram import TrackerHistogram
from camera.roi_tracker import ROITracker
from kivy.logger import Logger
from kivy.uix.boxlayout import BoxLayout
from kivy.uix.anchorlayout import AnchorLayout
from kivy.uix.floatlayout import FloatLayout
from kivy.uix.togglebutton import ToggleButton
from kivy.uix.togglebutton import Button
from kivy.uix.label import Label
from kivy.uix.scatter import Scatter
from kivy.uix.image import Image
from kivy.garden.graph import Graph, MeshLinePlot, MeshStemPlot
from kivy.uix.slider import Slider
from kivy.uix.progressbar import ProgressBar
from kivy.graphics.transformation import Matrix
from kivy.uix.popup import Popup
from time import sleep
from threading import Thread
import matplotlib
import RPi.GPIO as GPIO
import numpy as np
from kivy.app import App
from kivy.uix.carousel import Carousel
from kivy.uix.image import AsyncImage

matplotlib.use('module://kivy.garden.matplotlib.backend_kivyagg')
from matplotlib import pyplot as plt
from kivy.garden.matplotlib.backend_kivyagg import FigureCanvas
from time import sleep
import time
import datetime
import numpy as np
import cv2
import os

LED_Pin = 19 # Broadcom Pin 4

class IDSGUI(App):
    def __init__(self):
        super(IDSGUI, self).__init__()
        self._camera = None

    def _led_toggle(self, val):
        if self._led_state == GPIO.LOW:
            self._led_state = GPIO.HIGH
            self._toggle.text = "LED ON"
        else:
            self._led_state = GPIO.LOW
            self._toggle.text = "LED OFF"
        GPIO.output(LED_Pin, self._led_state)

    def _start_count(self, val):
        pass
        # fake_bin = int((np.random.randn() * 100) + 320)
        # self._tracker_histogram.add_track(max(0, min(640, fake_bin)))
        # (x,y) = self._camera.get_roi_offset()
        # self._camera.set_roi_offset(x+50,y+50)

    def _toggle_object_detection(self, val):
        pass
    #     detecting = not self._camera.get_object_detection()
    #     self._camera.set_object_detection(detecting)
    #     if not detecting:
    #         self._object_detection.source = self.img_path('object_detection_off.png')
    #     else:
    #         self._object_detection.source = self.img_path('object_detection_on.png')

    def _change_exposure(self, instance, val):
        pass
        # self._camera.set_exposure(int(val))

    def _do_reset_scatter(self, val):
        # move the
        mat = Matrix().scale(10,10,10).translate(0,-150,0)
        self._scatter.transform = mat

    def _exit_app(self, val):
        # self._camera.play = False
        # give it a 1/4 sec to shu down the camera
        sleep(0.25)
        App.get_running_app().stop()
        import sys
        sys.exit()

    def _update_histogram(self, val):
        pass
        # frame = self._camera.get_current_frame()
        # self._histogram.set_data(frame)


    def _update_fps(self, l):
        pass
        # self._fps.text = "FPS: %d" % self._camera.get_fps()
        # self._temp.text = "Temp: %s" % self._camera.get_temp()
        # self._exposure.text = "Exposure: %d" % self._camera.get_exposure()
        # self._centroid.text = "C: %d" % self._histogram.centroid

    def _update_roi(self, roi, roi2):
        pass
        # roi = self._roi_tracker.roi_Offset
        # self._camera.set_roi_offset(roi[0], roi[1])


    def img_path(self, image_name):
        return os.path.join(os.path.dirname(__file__), 'img', image_name)

    def init_GPIO(self):
        GPIO.setmode(GPIO.BCM)
        GPIO.setup(LED_Pin, GPIO.OUT)
        self._led_state =  GPIO.LOW
        GPIO.output(LED_Pin, self._led_state)

    def get_ip_address(self):
        try:
            import socket
            s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
            s.connect(("8.8.8.8", 80))
            addr =  s.getsockname()[0]
            s.close()
            return addr
        except:
            return "Error"

    def build(self):
        self.init_GPIO()

        layout = FloatLayout(size=(800, 480), pos=(0,0))

        self._toggle = ToggleButton(text='LED OFF',
           size_hint=(0.2,0.2), pos_hint={'pos':(0.8,0)})

        self._snap = Button(text='COUNT',
           size_hint=(0.2,0.2), pos_hint={'pos':(0.8,0.2)})

        self._object_detection = ImageButton(size_hint=(0.1,0.1), pos_hint={'pos':(0.7,0.0)},
            source=self.img_path('object_detection_off.png'))

        self._reset_scatter = ImageButton(size_hint=(0.1,0.1), pos_hint={'pos':(0.7,0.1)},
            source=self.img_path('reset_scatter.png'))

        self._logo = Image(size_hint=(0.3,0.2), pos_hint={'pos':(-0.03,0.84)},
            source=self.img_path('ids.png'))

        self._exit = ImageButton(size_hint=(0.05,0.05), pos_hint={'pos':(0.96,0.95)},
            source=self.img_path('close-white.png'))

        red, white, kivy_blue = (1,0,0,1), (1,1,1,1), (51./255, 164./255, 206./255, 1)


        self._fps = Label(text='FPS: 0', size_hint=(0.1,0.1),
                          pos_hint={'pos':(0.8,0.9)}, color=red)
        self._temp = Label(text='Temp: 0', size_hint=(0.1,0.1),
                          pos_hint={'pos':(0.6,0.9)}, color=white)


        dev_ip = self.get_ip_address()

        self._ip = Label(text='IP: %s'%dev_ip, size_hint=(0.1,0.1),
                          pos_hint={'pos':(0.4,0.9)}, color=white)
        self._exposure = Label(text='Exposure: 0', size_hint=(0.2,0.1),
                          pos_hint={'pos':(0,0)})

        self._centroid = Label(text='C:0', size_hint=(0.1,0.1),
                          pos_hint={'pos':(0.79,0.83)}, color=white)

        self._exposure_slider = Slider(min=0, max=2500, value=1358,
                          size_hint=(0.5,0.1),
                          pos_hint={'pos':(0.2,0)}  )


        # self._camera = Camera2131(resolution=(1280,960),
        #                           fourcc="GREY",
        #                           capture_resolution=(3872, 2764),
        #                           capture_fourcc="Y16 ",
        #                           size_hint=(1,1),
        #                           pos_hint={'pos':(0,0)},
        #                           play=True, )

        # self._camera = CameraIDS(resolution=(640, 480),
        #                           fourcc="GREY",
        #                           capture_resolution=(640, 480),
        #                           capture_fourcc="GREY",
        #                           size_hint=(1,1),
        #                           pos_hint={'pos':(0,0)},
        #                           play=True)

        self._histogram = Histogram(
            size_hint=(0.2,0.25), pos_hint={'pos':(0.8,0.65)})

        self._tracker_histogram = TrackerHistogram(
            size_hint=(0.2,0.25), pos_hint={'pos':(0.8,0.4)})

        # self._roi_tracker = ROITracker(self._camera.resolution,
        #     size_hint=(0.2,0.25), pos_hint={'pos':(0.02,0.6)})
        # self._roi_tracker.bind(roi_Offset=self._update_roi)

        # self._demo.bind(on_press=self._show_demo_results)
        self._toggle.bind(on_press=self._led_toggle)
        self._snap.bind(on_press=self._start_count)
        self._exit.bind(on_press=self._exit_app)
        self._object_detection.bind(on_press=self._toggle_object_detection)
        self._exposure_slider.bind(value=self._change_exposure)
        self._reset_scatter.bind(on_press=self._do_reset_scatter)

        # self._camera.fbind('on_frame_complete',self._update_histogram)

        self._scatter = Scatter(size_hint=(None, None), size=(200,200),)
        # self._scatter.add_widget(self._camera)
        # layoutInner = FloatLayout(size_hint=(0.8, 1), pos_hint={'x':0,'y':0})
        # layoutInner.add_widget(self._scatter)
        layout.add_widget(self._scatter)

        mat = Matrix().scale(10,10,10).translate(0,-150,0)
        self._scatter.apply_transform(mat)
        layout.add_widget(self._histogram)
        layout.add_widget(self._tracker_histogram)
        layout.add_widget(self._snap)
        layout.add_widget(self._exit)
        layout.add_widget(self._exposure_slider)
        layout.add_widget(self._object_detection)
        layout.add_widget(self._reset_scatter)

        layout.add_widget(self._exposure)
        layout.add_widget(self._fps)
        layout.add_widget(self._temp)
        layout.add_widget(self._ip)
        Clock.schedule_interval(self._update_fps, 2)
        layout.add_widget(self._toggle)
        layout.add_widget(self._logo)
        # layout.add_widget(self._roi_tracker)
        self._is_updating = False
        return layout

IDSGUI().run()

