# kiezpilz-monitor-jan22-2021

For arduion mkrwifi1010 with env sensor shield.

The arduino will write all data to a file called KPK.txt
as well as log data via telegram

The arduino will also restart every hour, to provide a layer of robustness against losing wifi.

for this to work, you also need an arduino_secrets.h file in the same directory like this:
```
#define SECRET_SSID "yourssid"                                      // network name
#define SECRET_PASS "yournetwork-pass"                                  // network password
// Telegram Bot informations
#define BOT_TOKEN "yourbottoken"  						  // Your BOT Token given by @BotFather
#define BOT_NAME "yourbottname"                            // type here the bot name given by @BotFather
#define BOT_USERNAME "your bot username"
```
