import csv
import xml.etree.ElementTree as ET
import base64
  
  
def parseXML(xmlfile):
  
    # create element tree object
    tree = ET.parse(xmlfile)
  
    # get root element
    root = tree.getroot()
  
    # create empty list for news items
    newsitems = []
  
    # iterate news items
    for item in root.findall('./img'):
        #print(item.attrib['src'])
        image = item.attrib['src']
        datapath = item.attrib['data-path']
        base64_img = image[image.find(',')+1:]

        base64_img_bytes = base64_img.encode('utf-8')
        with open(datapath, 'wb') as file_to_save:
            decoded_image_data = base64.decodebytes(base64_img_bytes)
            file_to_save.write(decoded_image_data)


        print (image)
        #png.from_array(imagebytes, 'L').save("small_smiley.png")

#print (xml)

parseXML('assets.xml')