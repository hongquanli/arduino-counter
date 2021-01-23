# set QT_API environment variable
import os 
os.environ["QT_API"] = "pyqt5"
import qtpy

# qt libraries
from qtpy.QtCore import *
from qtpy.QtWidgets import *
from qtpy.QtGui import *

import control.utils as utils
from control._def import *

from queue import Queue
import time
import numpy as np
import pyqtgraph as pg
from datetime import datetime
from pathlib import Path

class Waveforms(QObject):

    signal_plot1 = Signal(np.ndarray,np.ndarray)
    signal_ch1 = Signal(str)

    def __init__(self,microcontroller):
        QObject.__init__(self)
        self.file = open(str(Path.home()) + "/Downloads/" + datetime.now().strftime('%Y-%m-%d %H-%M-%-S.%f') + ".csv", "w+")
        # self.file.write('Time,Frequency (Hz)')
        self.microcontroller = microcontroller
        self.ch1 = 0
        self.time = 0
        self.timestamp = 0 # MCU timestamp, in microsecond
        self.ch1_array = np.array([])
        self.time_array = np.array([])
        self.signal_array = np.array([])
        self.timer_update_waveform = QTimer()
        self.timer_update_waveform.setInterval(MCU.DATA_INTERVAL_ms/2)
        self.timer_update_waveform.timeout.connect(self.update_waveforms)
        self.timer_update_waveform.start()

        self.first_run = True

        self.time_now = 0
        self.time_0 = time.time()

        self.counter_display = 0
        self.counter_file_flush = 0

        self.logging_is_on = True

    def logging_onoff(self,state,experimentID):
        self.logging_is_on = state
        if state == False:
            self.file.close()
        else:
            self.experimentID = experimentID
            self.file = open(str(Path.home()) + "/Downloads/" + self.experimentID + '_' + datetime.now().strftime('%Y-%m-%d %H-%M-%-S.%f') + ".csv", "w+")

    def update_waveforms(self):
        # self.time = self.time + (1/1000)*WAVEFORMS.UPDATE_INTERVAL_MS
      
        if SIMULATION:
            # test plotting multiple data points at a time
            #for i in range(MCU.TIMEPOINT_PER_UPDATE):
            t_chunck = np.array([])
            ch1_chunck = np.array([])

            for i in range(MCU.TIMEPOINT_PER_UPDATE):
                # Use the processor clock to determine elapsed time since last function call
                self.time = time.time() - self.time_0
                self.ch1 = (self.ch1 + 0.2/MCU.TIMEPOINT_PER_UPDATE)%5

                # append variables for plotting
                t_chunck = np.append(t_chunck,self.time)
                ch1_chunck = np.append(ch1_chunck,self.ch1)

            self.time_array = np.append(self.time_array,t_chunck)
            self.ch1_array = np.append(self.ch1_array,ch1_chunck)

            self.signal_plot1.emit(self.time_array[-500:],self.ch1_array[-500:])
            self.signal_ch1.emit("{:.2f}".format(self.ch1))


        else:
            readout = self.microcontroller.read_received_packet_nowait()
            if readout is not None:

                self.time_now = time.time()

                t_chunck = np.array([])
                ch1_chunck = np.array([])

                for i in range(MCU.TIMEPOINT_PER_UPDATE):
                    # time
                    timestamp_previous = self.timestamp
                    self.timestamp = int.from_bytes(readout[i*MCU.RECORD_LENGTH_BYTE:i*MCU.RECORD_LENGTH_BYTE+4], byteorder='big', signed=False)
                    if self.first_run:
                        self.timestamp_start = self.timestamp
                        self.first_run = False
                    else:
                        self.time = (self.timestamp - self.timestamp_start)*1e-6
                        self.ch1 = 1/((self.timestamp - timestamp_previous)*1e-6) # frequency in Hz

                        record_from_MCU = (
                            str(self.time) + '\t' + str(self.ch1) + '\t' )
                        record_settings = (str(self.time_now))
                       
                        # saved variables
                        if self.logging_is_on:
                            self.file.write(record_from_MCU + '\t' + record_settings + '\n')

                        # append variables for plotting
                        t_chunck = np.append(t_chunck,self.time)
                        ch1_chunck = np.append(ch1_chunck,self.ch1)

                self.ch1_array = np.append(self.ch1_array,ch1_chunck)
                self.time_array = np.append(self.time_array,t_chunck)

                # reduce display refresh rate
                self.counter_display = self.counter_display + 1
                if self.counter_display>=1:
                    self.counter_display = 0
                    self.signal_plot1.emit(self.time_array[-500:],self.ch1_array[-500:])
                    self.signal_ch1.emit("{:.2f}".format(self.ch1))

        # file flushing
        if self.logging_is_on:
            self.counter_file_flush = self.counter_file_flush + 1
            if self.counter_file_flush>=500:
                self.counter_file_flush = 0
                self.file.flush()

    def close(self):
        self.file.close()