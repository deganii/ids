from kivy.clock import Clock
from kivy.graphics.texture import Texture
from kivy.core.camera import CameraBase
import select

from PIL import Image
from threading import Thread
from kivy.logger import Logger
import sys
from time import sleep
import time
import datetime
import v4l2capture

import os
from subprocess import Popen, PIPE
import subprocess

import numpy as np
import cv2

# from TIS uvc-exctensions/usb3.xml
V4L2_EXPOSURE_TIME_US = 26862081 # 0x0199e201
V4L2_GAIN_ABS = 26862084 # 0x0199e204
V4L2_ROI_OFFSET_X = 26862104 # 0x0199e218
V4L2_ROI_OFFSET_Y = 26862105 # 0x0199e219
V4L2_ROI_AUTO_CENTER = 26862112 # 0x0199e220

class CoreCameraIDS(CameraBase):
    """Implementation of ImagingSource Camera
    """

    def __init__(self, **kwargs):
        kwargs.setdefault('fourcc', 'GRAY')
        # kwargs.setdefault('mode', 'L')
        self._user_buffer = None
        self._format = 'rgb'
        self._video_src = 'v4l'
        self._device = None
        self._texture_size = None
        self._fourcc = kwargs.get('fourcc')
        self._mode = kwargs.get('mode')
        self._capture_resolution = kwargs.get('capture_resolution')
        self._capture_fourcc = kwargs.get('capture_fourcc')
        self.capture_requested = False
        self.ref_requested = False
        self._exposure_requested = False
        self._requested_exposure = 0
        self._exposure = 0

        self._roi_requested = False
        self._requested_roi = (-1,-1)
        self._roi_offset = (-1, -1)

        self._object_detection = False
        self._fps = 0
        self._temp = ""
        if self._mode is None:
            self._mode = self._get_mode_from_fourcc(self._fourcc)

        super(CoreCameraIDS, self).__init__(**kwargs)

    def _get_mode_from_fourcc(self, fourcc):
            return "I;16" if fourcc == "Y16 " else "L"

    def init_camera(self):
        self._device = '/dev/video%d' % self._index
        if not self.stopped:
             self.start()

    def _do_capture(self, is_ref):
        try:
            device = self._device
            video = v4l2capture.Video_device(device)
            (res_x, res_y) = self._capture_resolution
            fourcc = self._capture_fourcc
            (size_x, size_y) = video.set_format(res_x, res_y, fourcc=fourcc)
            capture_texture_size = (size_x, size_y)
            video.create_buffers(1)
            video.queue_all_buffers()
            video.start()
            select.select((video,), (), ())
            image_data = video.read_and_queue()
            Logger.debug("Obtained a frame of size %d", len(image_data))
            image = Image.frombytes(self._get_mode_from_fourcc(fourcc),
                                    capture_texture_size, image_data)
            ts = time.time()
            st = datetime.datetime.fromtimestamp(ts).strftime('%Y-%m-%d-%Hh-%Mm-%Ss')
            if is_ref:
                file = '/home/pi/2.131-captures/reference-%s.tiff' % st
            else:
                file = '/home/pi/2.131-captures/capture-%s.tiff' % st
            image.save(file, format='TIFF')

            #self._user_buffer = image
            #Clock.schedule_once(self._capture_complete)
            video.close()

            # start the dropbox upload
            # todo: move this out of the camera code!
            # self.upload(file)
        except:
            e = sys.exc_info()[0]
            Logger.exception('Exception! %s', e)
            Clock.schedule_once(self.stop)

    def perform_mser(self, frame):
        def intersection(a, b):
            x = max(a[0], b[0])
            y = max(a[1], b[1])
            w = min(a[0] + a[2], b[0] + b[2]) - x
            h = min(a[1] + a[3], b[1] + b[3]) - y
            if w < 0 or h < 0: return 0
            return w * h

        mser = cv2.MSER_create(
            _delta=5,
            _min_area=60,
            _max_area=14400,
            _max_variation=0.25,
            _min_diversity=0.2,
            _max_evolution=200,
            _area_threshold=1.01,
            _min_margin=0.003,
            _edge_blur_size=5)

        Logger.info("1st MSER (Polystyrene)")
        frame = np.array(frame)
        gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
        vis = cv2.cvtColor(np.asarray(frame.copy()), cv2.COLOR_RGB2BGR)

        regions, q = mser.detectRegions(gray)

        # polylines
        hulls = [cv2.convexHull(p.reshape(-1, 1, 2)) for p in regions]
        # cv.polylines(vis, hulls, 1, (0, 255, 0))

        # boundingboxes
        mask = np.zeros((frame.shape[0], frame.shape[1], 1), dtype=np.uint8)
        mask = cv2.dilate(mask, np.ones((150, 150), np.uint8))
        Logger.info("Dilate complete")
        for contour in hulls:
            cv2.drawContours(mask, [contour], -1, (255, 255, 255), -1)

        bboxes = []
        for i, contour in enumerate(hulls):
            x, y, w, h = cv2.boundingRect(contour)
            bboxes.append(cv2.boundingRect(contour))
            cv2.rectangle(vis, (x, y), (x + w, y + h), (0, 255, 0), 2)
        image = Image.fromarray(cv2.cvtColor(vis, cv2.COLOR_BGR2RGB))
        return image, bboxes


    def _v4l_init_video(self):
        # sleep(1.0)
        device = self._device
        (res_x, res_y) = self.resolution
        fourcc = self._fourcc
        Logger.info("video_thread started")
        video = v4l2capture.Video_device(device)
        (size_x, size_y) = video.set_format(res_x, res_y, fourcc=fourcc)
        self._texture_size = (size_x, size_y)
        Logger.info("Received resolution: %d,%d", size_x, size_y)
        video.create_buffers(1)
        video.queue_all_buffers()
        video.start()
        self._reset_fps()
        return video

    def _v4l_loop(self):
        while True:
            try:
                video = self._v4l_init_video()
            except:
                e = sys.exc_info()[0]
                Logger.exception('Exception on video thread startup! %s', e)
                try:
                    if video is not None:
                        video.close()
                except:
                    e2 = sys.exc_info()[0]
                    Logger.info("Exception while trying to close video stream for retry... %s", e2)
                Logger.info("Trying to restart video stream")
                # Try again in a second...
                sleep(2.0)
            break # get out of the loop once this works...

        while not self.stopped:
            try:
                try:
                    # Logger.debug("Obtaining a frame...")
                    select.select((video,), (), ())
                    image_data = video.read_and_queue()
                except:
                    if not self._object_detection:
                        # don't rethrow if we're doing an anlysis...
                        raise
                # Logger.debug("Obtained a frame of size %d", len(image_data))
                image = Image.frombytes(self._mode, self._texture_size, image_data)
                self._user_buffer = image

                # convert to rgb for display on-screen
                while (self._buffer is not None):
                    # make this an event object?
                    sleep(0.02)

                #self._buffer = image.convert('RGB').tobytes("raw", "RGB")
                image = image.convert('RGB')

                # draw some hough circles on the RGB buffer as an overlay
                if self._object_detection:
                    try:
                        oldImg = image
                        image,bboxes = self.perform_mser(image)
                        # image = self.mser_part2(image,vis,bboxes)
                    except:
                        image = oldImg
                        e = sys.exc_info()[0]
                        Logger.exception('Analysis Exception! %s', e)

                # convert to RGB in order to display on-screen
                self._buffer = image.tobytes("raw", "RGB")
                self._fps_tick()

                Clock.schedule_once(self._update)

                self._exposure = video.get_exposure_absolute()
                self._roi_offset = (video.get_generic_int(V4L2_ROI_OFFSET_X),
                                     video.get_generic_int(V4L2_ROI_OFFSET_Y))

                if(self._exposure_requested):
                    video.set_exposure_absolute(self._requested_exposure)
                    self._exposure_requested = False

                if(self._roi_requested):
                    (x,y) = self._requested_roi
                    # turn off auto-center
                    video.set_generic_int(V4L2_ROI_AUTO_CENTER, 0)
                    print("ROI OFFSET BEFORE: ({0},{1})".format(self._roi_offset[0], self._roi_offset[1]))
                    video.set_generic_int(V4L2_ROI_OFFSET_X, x)
                    video.set_generic_int(V4L2_ROI_OFFSET_Y, y)
                    self._roi_requested = False

                    self._roi_offset = (video.get_generic_int(V4L2_ROI_OFFSET_X),
                                        video.get_generic_int(V4L2_ROI_OFFSET_Y))
                    print("ROI OFFSET After: ({0},{1})".format(self._roi_offset[0], self._roi_offset[1]))

                if(self.capture_requested or self.ref_requested):
                    # need to switch to high res mode
                    # video.close()
                    # self._do_capture(self.ref_requested)
                    ts = time.time()
                    st = datetime.datetime.fromtimestamp(ts).strftime('%Y-%m-%d-%Hh-%Mm-%Ss')
                    file = '/home/pi/2.131-captures/capture-%s.tiff' % st
                    image.save(file, format='PNG')

                    self.capture_requested = False
                    self.ref_requested = False
                    # reinitialize
                    # video = self._v4l_init_video()
            except:
                e = sys.exc_info()[0]
                Logger.exception('Exception! %s', e)
                if video is not None:
                    video.close()
                Logger.info("Trying to restart video stream")
                # Try again...
                sleep(1.0)
                video = self._v4l_init_video()

                #Clock.schedule_once(self.stop)
        Logger.info("closing video object")
        video.close()
        Logger.info("video_thread exiting")

    def _reset_fps(self):
        self.TICK_SAMPLES = 25
        self._ticksum = 0
        self._tickindex = 0
        self._tick_samples = np.zeros(self.TICK_SAMPLES)
        self._lasttime = time.time()
        self._fps = 0
        self._temp = 0

    def _read_temp(self):
        temp = os.popen("vcgencmd measure_temp").readline()
        temp = temp.replace("temp=", "").strip()
        return temp

    def _fps_tick(self):
        newtime = time.time()
        newtick = newtime - self._lasttime
        self._ticksum -= self._tick_samples[self._tickindex]
        self._ticksum += newtick
        self._tick_samples[self._tickindex] = newtick
        self._tickindex = (self._tickindex + 1) % self.TICK_SAMPLES
        self._fps = self.TICK_SAMPLES / self._ticksum
        self._temp = self._read_temp()
        self._lasttime = newtime

    def start(self):
        Logger.info("d3 camera start() called")
        super(CoreCameraIDS, self).start()
        t = Thread(name='video_thread',
                   target=self._v4l_loop)
        t.start()

    def stop(self, dt=None):
        super(CoreCameraIDS, self).stop()

    def get_current_frame(self):
        return self._user_buffer

    def capture__full_res_frame(self):
        self.capture_requested = True

    def capture__full_res_ref(self):
        self.ref_requested = True

    def get_fps(self):
        return self._fps

    def get_temp(self):
        return self._temp

    def set_roi_offset(self, x, y):
        self._requested_roi = (x, y)
        self._roi_requested = True

    def get_roi_offset(self):
        return self._roi_offset

    def set_exposure(self, val):
        self._requested_exposure = val
        self._exposure_requested = True

    def get_exposure(self):
        return self._exposure

    def get_total_upload_size(self):
        return self._total_upload_size

    def get_uploaded_size(self):
        return self._uploaded_size

    def set_object_detection(self, val):
        self._object_detection = val

    def get_object_detection(self):
        return self._object_detection

    def _update(self, dt):
        if self._buffer is None:
            return
        Logger.debug("Rendering a frame...")
        if self._texture is None and self._texture_size is not None:
            Logger.debug("Creating a new texture...")
            self._texture = Texture.create(
                size=self._texture_size, colorfmt='rgb')
            self._texture.flip_vertical()
            self.dispatch('on_load')
        self._copy_to_gpu()

    def on_texture(self):
        pass

    def on_load(self):
        pass

