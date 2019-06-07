from kivy.uix.image import Image
from kivy.properties import NumericProperty
from kivy.clock import Clock

import numpy as np
from kivy.logger import Logger
from kivy.garden.graph import Graph, MeshLinePlot, MeshStemPlot

# import matplotlib
# matplotlib.use('module://kivy.garden.matplotlib.backend_kivy')
# from matplotlib import pyplot as plt
# from matplotlib.figure import Figure
# from kivy.garden.matplotlib.backend_kivyagg import FigureCanvas
import sys
import numpy as np
from scipy.stats import binned_statistic

class TrackerHistogram(Graph):
    """Implementation of a histogram kivy component for object tracking
        Based on opencv and kivy-garden graph libraries
    """

    def __init__(self, **kwargs):
        kwargs['xlabel'] = ''
        kwargs['ylabel'] = ''
        kwargs['x_ticks_minor'] = 2
        kwargs['y_ticks_minor'] = 0
        kwargs['x_ticks_major'] = 6
        kwargs['y_ticks_major'] = 0

        kwargs['x_grid_label'] = True
        kwargs['y_grid_label'] = False
        kwargs['x_grid'] = False
        kwargs['y_grid'] = False
        kwargs['xmin'] = 0
        kwargs['xmax'] = 64
        kwargs['ymin'] = 0
        kwargs['ymax'] = 1
        kwargs['padding'] = '5'

        super(TrackerHistogram, self).__init__(**kwargs)
        self.fbind('bins', self._on_bins_changed)
        self._hist_plot = MeshStemPlot()
        self.add_plot(self._hist_plot)
        self._raw_values = []

    bins = NumericProperty(64)
    '''Number of bins in the histogram (640 by default)
    '''

    def add_track(self, value):
        # generate a histogram from a PIL image
        try:
            self._raw_values.append(value)
            #display_bins = binned_statistic(self._raw_bins, self._raw_bins, bins=64, range=(0, 1))[0]
            np_bins = np.histogram(self._raw_values, 64, range=(0,640), density=False)[0].astype(np.float)
            print(np_bins/np.max(np_bins))
            self._hist_plot.points = enumerate(np_bins/np.max(np_bins))
        except:
            e = sys.exc_info()[0]
            Logger.exception('Exception! %s', e)
            return None

    def reset(self, bin):
        self._raw_bins = []
        self._hist_plot.points = [(i,0) for i in range(0, self.bins)]

    def _on_bins_changed(self, *largs):
        self.xmax = self.bins - 1
