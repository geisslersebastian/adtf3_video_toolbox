import QtQuick 2.9
import QtQuick.Controls 1.4
import QtQuick.Dialogs 1.2
import QtQuick.Layouts 1.1
import Adtf 1.0

Item
{
  	id: root
    property var labels;
    property var classes: ["person","bicycle","car","motorbike","aeroplane","bus","train","truck","boat","traffic light","fire hydrant","stop sign","parking meter","bench","bird","cat","dog","horse","sheep","cow","elephant","bear","zebra","giraffe","backpack","umbrella","handbag","tie","suitcase","frisbee","skis","snowboard","sports ball","kite","baseball bat","baseball glove","skateboard","surfboard","tennis racket","bottle","wine glass","cup","fork","knife","spoon","bowl","banana","apple","sandwich","orange","broccoli","carrot","hot dog","pizza","donut","cake","chair","sofa","pottedplant","bed","diningtable","toilet","tvmonitor","laptop","mouse","remote","keyboard","cell phone","microwave","oven","toaster","sink","refrigerator","book","clock","vase","scissors","teddy bear","hair drier","toothbrush"];
 
  	Component.onCompleted: 
    {
        var video = filter.createInputPin("video")
        video.sample.connect(function(sample)
        {
          	imageContainer.image = sample.data
          	gc()
        })

        var dnn = filter.createInputPin("dnn")
        dnn.sample.connect(function(sample)
        {
            var b = new Float32Array(sample.data);
          
          	var w = 85
            var h = 507
            
            if(b.length != w * h)
            {
                console.log("Not supported Net")
            }

            var labels = [];
            for (var i = 0; i < b.length; i += w)
            {
              var confidence = 0.0;
              var classid = 0;

              for(var j = 5; j < w; j++)
              {
                var c = b[i + j] + 0.0;

                if(confidence < c)
                {
                  //log.info("Confidence: " + c + " " + j + " " + root.classes[j]);
                  confidence = c;
                  classid = j-5;
                }
              }
              console.log("Confidence " + confidence);
              if (confidence > 0.2)
              {
                //log.info("Confidence " + confidence);

                var o = {
                  center_x: b[i + 0] + 0.0,
                  center_y: b[i + 1] + 0.0,
                  width: b[i + 2] + 0.0,
                  height: b[i + 3] + 0.0,
                  confidence: confidence,
                  class: classid + ":" + root.classes[classid]
                };

                if(labels.length < 6)
                {
                  labels.push(o);
                }
              }
            }

            root.labels = labels;
        })
    }

  anchors.fill: parent
  
  Item
  {
    id: container
    anchors.fill: parent;
    
    ImageItem
    {
      id: imageContainer
      anchors.fill: parent;
      anchors.margins: 40;
    }

    Repeater
    {
      model: root.labels ? root.labels.length: 0;

      Rectangle
      {
        id: l
        property var m: root.labels[index];

        border.width: 2
        border.color: "green"
        color: "transparent"

        x: imageContainer.width * l.m.center_x + 40 - l.width / 2 
        y: imageContainer.height * l.m.center_y + 40 - l.height / 2

        width:  imageContainer.width * l.m.width
        height: imageContainer.height * l.m.height

        Rectangle
        {
          color: "steelblue"

          width: label.width;
          height: label.height

          Label
          {
            id: label;
            text: m.class + " " + Number(m.confidence*100.0).toFixed(1);
          }  
        }

      }
    }
  }
}
