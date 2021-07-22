#include <Arduino.h>
#include <ESPmDNS.h>
#include "DHT.h"
#include "WiFi.h"
#include "ESPAsyncWebServer.h"

const char *ssid = "CoCoLabor";
const char *password = "cocolabor1234";
AsyncWebServer server(80);

#define DHTPIN 14
#define DHTTYPE DHT11
#define PIEZO 26
#define LIGHT 34
#define BUTTON 16

DHT dht(DHTPIN, DHTTYPE);
boolean wasOpened = false;
int buttonState = 0;

String getWasOpened()
{
  String siegel;
  if (wasOpened)
  {
    siegel = "ist gebrochen";
  }
  else
  {
    siegel = "ist aktiviert";
  }
  return siegel;
}

String readDHTTemperature()
{
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  // Read temperature as Celsius (the default)
  float t = dht.readTemperature();

  Serial.println(t);
  return String(t);
}

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html>

<head>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <link rel="stylesheet" href="https://use.fontawesome.com/releases/v5.7.2/css/all.css"
        integrity="sha384-fnmOCqbTlWIlj8LyTjo7mOUStjsKC4pOpQbqyi7RrhN7udi9RwhKkMHpvLbHG9Sr" crossorigin="anonymous">
    <script src="https://cdnjs.cloudflare.com/ajax/libs/moment.js/2.22.2/moment.min.js"></script>
    <script src="https://cdnjs.cloudflare.com/ajax/libs/Chart.js/2.7.2/Chart.min.js"></script>
    <script
        src="https://github.com/nagix/chartjs-plugin-streaming/releases/download/v1.5.0/chartjs-plugin-streaming.min.js"></script>
</head>
<style>
    html {
        font-family: Arial;
        display: inline-block;
        margin: 0px auto;
        text-align: center;
    }

    h2 {
        font-size: 2.0rem;
    }

    p {
        font-size: 3.0rem;
    }

    .units {
        font-size: 1.2rem;
    }

    .dht-labels {
        font-size: 1.5rem;
        vertical-align: middle;
        padding-bottom: 15px;
    }

    #container {
        display: flex;
        flex-direction: column;
        align-items: center;
    }

    #chart-container {
        width: 500px;
        height: 300px;
    }

    #header {}
</style>
</head>

<body>
    <div id="container">
        <div id="header">
            <h2>Medical Distribution Service</h2>
            <h3>Temperature Tracking</h3>
        </div>
        <p>
            <i class="fas fa-thermometer-half" style="color:#059e8a;"></i>
            <span class="dht-labels">Temperature</span>
            <span id="temperature">%TEMPERATURE%</span>
            <sup class="units">&deg;C</sup>
        </p>
        <p>
            <i class="fas fa-stamp" style="color:#059e8a;"></i>
            <span class="dht-labels">Siegel</span>
            <span id="siegel">%SIEGEL%</span>
        </p>
        <div id="chart-container">
            <canvas id="myChart"></canvas>
        </div>
        <a id="downloadAnchorElem" style="display:none"></a>
    </div>
</body>
<script>
    let temperature;
    let sensorDataArray = [];
    var chartColors = {
        blue: 'rgb(54, 162, 235)',
    };

    function onRefresh(chart) {
        
        getTemperature();
        var sensorData = {
            x: Date.now(),
            y: temperature
        }

        var i = 0;
        console.log(JSON.stringify(sensorDataArray[i]));
        i++;
        sensorDataArray.push(sensorData);
        chart.config.data.datasets.forEach(function (dataset) {
            dataset.data.push({
                x: sensorData.x,
                y: sensorData.y
            });
        });
    }


      var dataStr = "data:text/json;charset=utf-8," + encodeURIComponent(JSON.stringify(sensorDataArray));
      var dlAnchorElem = document.getElementById('downloadAnchorElem');
      dlAnchorElem.setAttribute("href", dataStr);
      dlAnchorElem.setAttribute("download", "temperature.json");

    var color = Chart.helpers.color;
    var config = {
        type: 'line',
        data: {
            datasets: [{
                label: 'Temperature',
                backgroundColor: color(chartColors.blue).alpha(0.5).rgbString(),
                borderColor: chartColors.blue,
                fill: false,
                cubicInterpolationMode: 'monotone',
                data: []
            }]
        },
        options: {
            title: {
                display: true,
                text: 'temperature over time'
            },
            scales: {
                xAxes: [{
                    type: 'realtime'
                }],
                yAxes: [{
                    scaleLabel: {
                        display: true,
                        labelString: 'Temperature in °C'
                    }
                }]
            },
            tooltips: {
                mode: 'nearest',
                intersect: false
            },
            hover: {
                mode: 'nearest',
                intersect: false
            },
            plugins: {
                streaming: {
                    duration: 20000,
                    refresh: 1000,
                    delay: 2000,
                    onRefresh: onRefresh
                }
            }
        }
    };

    window.onload = function () {
        var ctx = document.getElementById('myChart').getContext('2d');
        window.myChart = new Chart(ctx, config);
    };

    function getTemperature() {
        var xhttp = new XMLHttpRequest();

        xhttp.onreadystatechange = function () {
            if (this.readyState == 4 && this.status == 200) {
                document.getElementById("temperature").innerHTML = this.responseText;
                temperature = this.responseText;
            }
        };
        xhttp.open("GET", "/temperature", true);
        xhttp.send();
    }

    setInterval(function () {
        var xhttp = new XMLHttpRequest();
        xhttp.onreadystatechange = function () {
            if (this.readyState == 4 && this.status == 200) {
                document.getElementById("siegel").innerHTML = this.responseText;
                if (this.responseText === "ist gebrochen") {
                    dlAnchorElem.click();
                }
            }
        };
        xhttp.open("GET", "/siegel", true);
        xhttp.send();
    }, 1000);
</script>

</html>)rawliteral";

// Replaces placeholder with DHT values
String processor(const String &var)
{
  //Serial.println(var);
  if (var == "TEMPERATURE")
  {
    return readDHTTemperature();
  }
  else if (var == "SIEGEL")
  {
    return getWasOpened();
  }
  return String();
}

void setup()
{
  // Serial port for debugging purposes
  Serial.begin(9600);
  dht.begin();

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }

  // Print ESP32 Local IP Address
  Serial.println(WiFi.localIP());

  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send_P(200, "text/html", index_html, processor); });
  server.on("/temperature", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send_P(200, "text/plain", readDHTTemperature().c_str()); });
  server.on("/siegel", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send_P(200, "text/plain", getWasOpened().c_str()); });

  if (!MDNS.begin("temperature-station"))
  {
    Serial.println("Error setting up MDNS responder!");
    while (1)
    {
      delay(1000);
    }
  }
  Serial.println("mDNS responder started");

  // Start TCP (HTTP) server
  server.begin();
  Serial.println("TCP server started");

  // Add service to MDNS-SD
  MDNS.addService("http", "tcp", 80);

  pinMode(PIEZO, OUTPUT);
  pinMode(BUTTON, INPUT);
}

void loop()
{
  float temperatureInCelcius = dht.readTemperature();
  int lightValue = analogRead(LIGHT);
  buttonState = digitalRead(BUTTON);

  Serial.println(lightValue);

  if (temperatureInCelcius > 25)
  {
    digitalWrite(PIEZO, HIGH);
    delay(100);
    digitalWrite(PIEZO, LOW);
    delay(100);
  }

  if (lightValue > 100)
  {
    if (!wasOpened)
    {
      wasOpened = true;

      digitalWrite(PIEZO, HIGH);
      delay(100);
      digitalWrite(PIEZO, LOW);
      delay(50);
      digitalWrite(PIEZO, HIGH);
      delay(100);
      digitalWrite(PIEZO, LOW);
    }
  }

  if (buttonState == HIGH)
  {
    wasOpened = false;
  }
}