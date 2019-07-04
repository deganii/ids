
import os
os.environ['KIVY_GL_BACKEND'] = 'gl'
os.environ['KIVY_BCM_DISPMANX_LAYER'] = "0"
import kivy

kivy.require('1.11.0') # replace with your current kivy version !


# from kivy.app import App
# from kivy.uix.label import Label
#
#
# class MyApp(App):
#
#
#     def build(self):
#         label = Label(text='Hello world')
#         l
#         return label
#
#
# if __name__ == '__main__':
#     MyApp().run()

#
# from kivy.lang import Builder
# from kivy.base import runTouchApp
# from kivy.core.window import Window
# #Window.size = (300, 100)
# from kivy.config import Config
# Config.set('graphics', 'resizable', '0') #0 being off 1 being on as in true/false
# Config.set('graphics', 'width', '200')
# Config.set('graphics', 'height', '480')
# Config.set('graphics', 'left', '600')
# Config.set('graphics', 'top', '0')
#
# runTouchApp(Builder.load_string('''
# BoxLayout:
#     canvas:
#         Color:
#             rgba: 1, 0, 0, 1
#         Rectangle:
#             size: 200,480
#             pos: 600,0
#     Button:
#         background_color: 0, 0, 0, 0
#         text: 'blob'
# '''))
#

from kivy.animation import Animation
from kivy.app import App
from kivy.uix.button import Button


# class TestApp(App):
#
#     def animate(self, instance):
#         # create an animation object. This object could be stored
#         # and reused each call or reused across different widgets.
#         # += is a sequential step, while &= is in parallel
#         animation = Animation(pos=(100, 100), t='out_bounce')
#         animation += Animation(pos=(200, 100), t='out_bounce')
#         animation &= Animation(size=(500, 500))
#         animation += Animation(size=(100, 50))
#
#         # apply the animation on the button, passed in the "instance" argument
#         # Notice that default 'click' animation (changing the button
#         # color while the mouse is down) is unchanged.
#         animation.start(instance)
#
#     def build(self):
#         # create a button, and  attach animate() method as a on_press handler
#         button = Button(size_hint=(None, None), text='plop',
#                         on_press=self.animate)
#         return button
#
#
# if __name__ == '__main__':
#     TestApp().run()

from kivy.logger import Logger

class AppButton(Button):

    # Handle touch - an up event

    def on_touch_down(self, touch):
        print("DOWN")
        Logger.info('DOWN')
        if self.collide_point(*touch.pos):

            self.text = "Touch down"

            # digest the event

            return False



    # Handle touch - a down event

    def on_touch_up(self, touch):
        print("UP")
        Logger.info('UP')
        if self.collide_point(*touch.pos):

            self.text = "Touch me...."

            # touch is on the button...do not want to send this event any further

            return False





# App class

class TouchDemo(App):

    def build(self):
        appButton = AppButton(text = "Touch me...")
        return appButton



if __name__ == '__main__':
    TouchDemo().run()