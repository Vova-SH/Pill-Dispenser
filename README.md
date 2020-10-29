# Automatic Pill Dispenser
ESP-IDF v4.1 (with PlatformIO IDE)

No third-party dependencies
## Base functionality
- Connecting to previous WiFi
- Reconnecting to WiFi using WPS
- HTTP Server for: web page admin panel, RESTful API and Digest Authentification
- NTP server for synchronize time
- Scheduler
- SPIFFS memory for saving all settings and schedule
## About schedule
You need to set a schedule for dispensing pills. This software allows to do this with an accuracy of a minute. You can also configure the following options:
- Start with… This option requires the date and time from when this task takes effect
- Repeat mode. This option has a next values: every day, every week, none. If option not equals "none", then after starting task setup new start time (depending on the selected option)
## RESTful API
If you want to write on over a processing server or client application, then you can use the next API. For input and output data format use a JSON objects.
#### HTTP Authentification and general errors
For all requests use [Digest Authentification](https://wikipedia.org/wiki/Digest_access_authentication) with only MD5 hashing. This API supported next errors status code:
| Status code | Description |
|:-:|:- |
| `401` | Unauthorized. This status returns, if your parameter "Authorization" in header not found or incorrect |
| `404` | Not found. Check correct your request |
| `500` | Internal server error. Maybe if your header very long or internal api error |

With the correct request or processing in all cases, returning `200` status code.
**Attention!** All `PUT` request always response similar JSON with current values.
#### Main settings
##### Distance lock
This setting shows current status dispensing hole. This is necessary if you want to lock up getting current pills.

`GET` `PUT` **/api/lock**
Request and response:
```json
{ "enable": true }
```
##### Light notification
This setting turns on the light after changing the sector with pills.

`GET` `PUT` **/api/light**
Request and response:
```json
{ "enable": true }
```
#### Time settings
##### Current time
This request is needed for getting current time on device. The value of time in seconds since 1970 January 1 00:00.

`GET` **/api/time**
Response:
```json
{ "time_sec": 1234 }
```
##### Setup manual time
The value of time in seconds since 1970 January 1 00:00.

`PUT` **/api/time**
Request (and response):
```json
{ "time_sec": 1234 }
```
##### NTP server url
Get and setup url NTP server.

`GET` `PUT` **/api/time/ntp**
Request and response:
```json
{ "name": "pool.ntp.org" }
```
##### NTP auto sync
Auto sync time with NTP every hour.

`GET` `PUT` **/api/time/ntp/auto**
Request and response:
```json
{ "enable": true }
```
##### NTP sync
This method is necessary if you want to sync it with NTP right now.

`POST` **/api/time/ntp/sync**
#### Security settings
##### Change WiFi setting
You can use this method to connect to a different access point or a non-WPS access point. After sending the request, the device tries to connect to the new access point and doesn't return a response.

`PUT` **/api/wifi**
Request:
```json
{
    "ssid" : "MyWiFi",
    "password" : "MySecretPassword"
}
```
##### Get WiFi SSID
This method returns current SSID access point.

`GET` **/api/wifi**
Request:
```json
{ "ssid" : "MyWiFi" }
```
##### Change Login form
If you want to change login and password for authentication, you can use this request. Your digest authentication must be invalid after this request is successfully completed.

`PUT` **/api/auth**
Request:
```json
{
    "login" : "MyLogin",
    "old_password" : "MySecretOldPassword",
    "new_password" : "MySecretNewPassword"
}
```
