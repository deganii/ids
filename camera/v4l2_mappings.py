import xml.etree.ElementTree as ET
import urllib.request
from bs4 import BeautifulSoup
# properties supported by DMM 24UJ003-ML / Firmware version 508

TIS_SPECIFICATION = 'https://raw.githubusercontent.com/TheImagingSource/tiscamera/master/data/uvc-extensions/usb3.xml'
usb3xml = urllib.request.urlopen(TIS_SPECIFICATION).read()
usb3xml = usb3xml.decode('utf-8')
soup = BeautifulSoup(usb3xml, 'html.parser')


propList = ['Exposure Time (us)',
            'Gain (dB/100)',
            'Trigger Mode',
            'Software Trigger',
            'Trigger Delay',
            'Strobe Enable',
            'Strobe Polarity',
            'Strobe Exposure',
            'Strobe Duration',
            'Strobe Delay',
            'GPOUT',
            'GPIN',
            'ROI Offset X',
            'ROI Offset Y',
            'ROI Auto Center',
            'Override Scanning Mode',
            'Trigger Global Reset Release']
root = ET.fromstring(usb3xml)
constants = {}
mappings = {}
defines = []
for const in soup.find_all('constant'):
    constants[const.id.string] = const.value.string
for mapping in soup.find_all('mapping'):
    mappings[mapping.find('name').string] = (mapping.v4l2.id.string, mapping.v4l2.v4l2_type.string)
for prop in propList:
    (v4l2_name,type) = mappings[prop]
    ctrl_id = constants[v4l2_name]
    defines.append('#define TIS_{0:30} {1}   // {2}'.format(v4l2_name, ctrl_id, type))
print('\n'.join(defines))