# E-paper ESP32 Frame for Waveshare 13.3" Spectra 6 screen

- **This project is currently a Work in Progress (WIP)!**

This requires an ESP32 with PSRAM due to the size of the images for the larger screen.

This project leverages **Immich** as a service for organizing albums and photos. Photos intended for display are grouped into specific albums, and a FLASK server hosted on a NAS or cloud server handles image cropping and editing before sending them to the ESP32. Since the ESP32 remains in deep sleep most of the time, and all image processing is handled by the server, the EPD updates photos very quickly, typically within 15 seconds. This significantly reduces power consumption.

## Features

- **Captive portal**: By long-pressing setup button on the ESP32 when boot up, the device enters setup mode, allowing the Wi-Fi setup page to store up to five SSIDs. This enhances mobility and makes it easier to switch between networks.
Mostly modifieded from TRMNL WiFiCaptive[https://github.com/usetrmnl/firmware/tree/main/lib/wificaptive]
- **Fully Automated Photo Management**: Manage photos through Immich without additional manual processes; photos will automatically sync to the frame.
- **Ultra-low Power Consumption**: As all image processing and quantization are handled by the server, the device only consumes ~16µA during deep sleep, with photo updates completed within 30 seconds.
- **Customizable Display**: Configure photo orientation, basic color adjustments, album name, and more through the server webpage.
- **Cython impelementation**: Use Cython to significantly accelerate photo processing, achieving up to a 5x speed boost.
- **HTTPS supported**: ESP32 now can connect to secured server.
- **Sleep time impelementation**: ESP32 will enter deep sleep during the specified sleep period.
- **One button**: When the ESP32 is in deep sleep, a short press of the setting button will wake it up and restart the process, while a long press(~5s) of the setting button during boot will enter setting mode.

## Table of Contents

- [Components](#components)
- [Installation](#installation)
- [License](#license)

## Components

- [Olimex ESP32-S3 DevKit LiPo](https://www.olimex.com/Products/IoT/ESP32-S3/ESP32-S3-DevKit-Lipo/open-source-hardware)
- [13.3-inch E Ink Spectra 6 (E6) Full Color E-Paper Display Module + HAT](https://www.waveshare.com/13.3inch-e-paper-hat-plus-e.htm)
- Picture frame: A standard picture frame that accommodates the e-paper frame.
- Li-Po battery with PH2.0 header
- Simple button for wake and setting

## Installation

### Clone the Repository

```bash
$ git clone https://github.com/biohead/epf-epd13in3e.git
```

### Manually Build Docker Image

```bash
$ git clone https://github.com/biohead/epf-epd13in3e.git
$ docker build -t biohead/epf-epd13in3e .
```

### Download Precompiled Docker Image ***NOT AVAILABLE for 13.3"***

If you prefer not to build the image yourself, you can download the precompiled image from [DockerHub](https://hub.docker.com/r/jwchen119/epf):

```bash
docker pull jwchen119/epf
```

### Run the Container

Create a container from the image. Don’t forget to edit your Immich API key.

```bash
$ docker run --name epf -e IMMICH-API-KEY='<replace-your-immich-api-key>' -d -p <replace-port>:5000 jwchen119/epf
```

### Configure `config.yaml` (no longer needed, configure the settings directly from webpage)
<details>
Below is an example of a configured `config.yaml` file:

```yaml
immich:
  # Album name, must match the album name created in Immich
  album: testAlbme
  # Photo rotation angle, accepts only (0, 90, 180, 270)
  rotation: 270
  # Immich server URL
  url: http://192.168.100.36:2283
  # Color(Saturation) enhancement level using PIL's ImageEnhance.Color (1.0 = original level)
  enhanced: 1.5
  # Contrast level using PIL's ImageEnhance.Contrast (1.0 = original level)
  contrast: 1.2
```
</details>

### ESP32-S3

Connect the EPD, ESP32-S3, Li-Po battery, and setting button according to the correct wiring configuration. 
To run the code follow the following steps:

1. Install and set up Arduino IDE
2. Connect your ESP32-S3
3. Rename the Arduino folder from the repo to `epd13in3e`
4. Open the `epd13in3e.ino` file
5. Install following libraries from Arduino library manager:
  5-1. Arduinojson
  5-2. Async TCP
  5-3. ESP Async Web Server
6. Click 'Upload'
7. Connect to the Wifi AP created by the ESP32, named `ESP32_ePAPER`
8. A captive portal shows up allowing to enter your WiFi details and details of the Docker container (e.g. http://192.168.100.10:15151)

You can re-enter the configuration page later by short-circuiting the setting button at least 5 second while rebooting.

## License

This project is licensed under the MIT License.

