from kivy.uix.behaviors import ButtonBehavior
from kivy.uix.widget import Widget
from kivy.properties import NumericProperty, ListProperty
from kivy.graphics import Color, Ellipse, Rectangle
from kivy.uix.behaviors import DragBehavior

SENSOR_FULL_WIDTH  = 3872.
SENSOR_FULL_HEIGHT = 2764.
CROP_WIDTH  = 640
CROP_HEIGHT = 480

class ROITracker(Widget):
    # roi_OffsetX = NumericProperty(0, min=0, max=3776)
    # '''Camera ROI Offset X
    # '''
    #
    # roi_OffsetY = NumericProperty(0, min=0, max=2668)
    # '''Camera ROI Offset Y
    # '''

    roi_Offset = ListProperty([0, 0])

    def __init__(self, resolution, **kwargs):
        super().__init__(**kwargs)
        self.touch = (0,0)
        self.bind(size=self.update)
        self.bind(pos=self.update)

        self.bind(pos=self.update,
                  size=self.update)

        self.window = resolution
        self.active_roi = ActiveROI(pos=(0,0))
        self.add_widget(self.active_roi)


    def update(self, *args):
        (w,h) = self.size
        (x,y) = self.pos
        aspect = w/h
        if aspect > 1.4: # limiting factor is height
            self.asp_scale = h / SENSOR_FULL_HEIGHT
        else:
            self.asp_scale = w / SENSOR_FULL_WIDTH
        # print("Size: ({0},{1})".format(*self.size))

        # print("Scale: {0} ".format(scale))
        print("Widget Pos: ({0},{1})".format(*self.pos))
        # self.roi_OffsetX = int((self.touch[0] - x) / self.asp_scale)
        # self.roi_OffsetY = int((self.touch[1] - y) / self.asp_scale)
        self.roi_Offset = [int((self.touch[0] - x) / self.asp_scale),
                           int((self.touch[1] - y) / self.asp_scale)]
        # print("ROI Offsets: ({0},{1})".format(self.roi_Offset[0],self.roi_Offset[1]))
        # self.canvas.clear()

        with self.canvas.before:
            Color(0, 0, 1)
            Rectangle(pos=(x, y), size=(self.asp_scale * SENSOR_FULL_WIDTH,
                                        self.asp_scale * SENSOR_FULL_HEIGHT))
            # Color(1, 0, 0)
            # print("Rect Size: ({0},{1})".format(scale * 640, scale * 480))
            # print("Rect Pos: ({0},{1})".format(x+self.roi_OffsetX, y+self.roi_OffsetY))
            # Rectangle(pos=self.touch, size=(self.asp_scale * self.window[0], self.asp_scale * self.window[1]))


    # def on_touch_down(self, touch):
    #     if self.collide_point(*touch.pos):
    #         # touch.push()
    #         # print("Touch Before: ({0},{1})".format(touch.x, touch.y))
    #         # touch.apply_transform_2d(self.to_local)
    #         # print("Touch After: ({0},{1})".format(touch.x, touch.y))
    #         self.touch = (touch.x,touch.y)
    #         # touch.pop()
    #         self.update()


class ActiveROI(DragBehavior, Widget):
    def __init__(self, **kwargs):
        super().__init__(**kwargs)
        with self.canvas:
            Color(1, 0, 0)
            self.rect = Rectangle(pos=self.pos, size=self.size)

    def update(self):
        with self.canvas:
            Color(1, 0, 0)
            Rectangle(pos=self.pos, size=self.size)
