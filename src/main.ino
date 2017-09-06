// Expose Espressif SDK functionality
extern "C" {
#include "user_interface.h"
}

#include <ESP8266WiFi.h>
#include "./structures.h"
#include "./configuration.h"

// TODO: make this dynamic
unsigned char requested_ssid[] = WIFI_SSID;

unsigned int channel = 1;
unsigned int found_channel = 0;
uint8_t found_bssid[ETH_MAC_LEN];

struct cache_entry {
    uint8_t mac[ETH_MAC_LEN];
    int rssi_total;
    int total;
};

#define MAX_ENTRIES 100
cache_entry cache_entries[MAX_ENTRIES];
int cache_entry_size = 0;

void find_requested_channel_cb(uint8_t *buf, uint16_t len)
{
    if (len == 128) {
        // beacon broadcast - grab the bssid if the ssid matches
        struct sniffer_buf2 *sniffer = (struct sniffer_buf2*) buf;
        struct beaconinfo beacon = parse_beacon(sniffer->buf, 112, sniffer->rx_ctrl.rssi);
        if( !memcmp(requested_ssid, beacon.ssid, sizeof(requested_ssid)) ) {
            Serial.printf("Found ssid %s on channel %d\r\n", beacon.ssid, beacon.channel);
            memcpy( found_bssid, beacon.bssid, ETH_MAC_LEN );
            found_channel = beacon.channel;
            wifi_set_channel(found_channel);
            wifi_set_promiscuous_rx_cb(get_client_rssi_cb);
        }
    }
}

void get_client_rssi_cb(uint8_t *buf, uint16_t len)
{
  if (len == 12) {
    // unknown
  } else if (len == 128) {
    // beacon broadcast - ignore
  } else {
    struct sniffer_buf *sniffer = (struct sniffer_buf*) buf;
    // NOTE: Doesn't seem to handle stuff when we have really quick networking from eg laptops

    //Is data packet
    if ((sniffer->buf[0] & 0x08) == 0x08) {
      struct clientinfo ci = parse_data(sniffer->buf, 36, sniffer->rx_ctrl.rssi, sniffer->rx_ctrl.channel);

      // if it is not the station broadcasting and it is the ssid that we were tracking
      if ( memcmp(ci.bssid, ci.station, ETH_MAC_LEN) && !memcmp( ci.bssid, found_bssid, ETH_MAC_LEN ) ) {
            int found = -1;

            for( int i = 0; i < cache_entry_size; i++ ) {
                if( !memcmp(ci.station, cache_entries[i].mac, ETH_MAC_LEN ) ) {
                    found = i;
                    break;
                }
            }
            if( found < 0 ) {
                found = cache_entry_size++;
                if( found >= MAX_ENTRIES )  // prevent overflow
                    return;

                memcpy( cache_entries[found].mac, ci.station, ETH_MAC_LEN );
                cache_entries[found].rssi_total = 0;
                cache_entries[found].total = 0;
            }

            cache_entries[found].rssi_total += sniffer->rx_ctrl.rssi;
            cache_entries[found].total++;
      }
    }
  }
}

void setup() {
    Serial.begin(115200);
    // CHIP doesn't catch stuff printed from this routine when triggering a soft reset if we dont have a bit of a delay here. 10ms isnt enough
    delay(200);
    Serial.printf("\n\nSDK version:%s\n\r", system_get_sdk_version());

    WiFi.setPhyMode(WIFI_PHY_MODE_11N);
    wifi_set_opmode(STATION_MODE);            // Promiscuous works only with station mode
    wifi_set_channel(channel);

    wifi_promiscuous_enable(0);
    wifi_set_promiscuous_rx_cb(find_requested_channel_cb);
    wifi_promiscuous_enable(1);
}

void loop() {
  wifi_set_channel(channel);

  int loops = 0;

  // Hop between channels looking for communications until we find the network we are looking for
  while (!found_channel) {
    if (loops++ > 200) {
      loops = 0;
      channel++;
      if (channel == 15)
        channel = 1;
      wifi_set_channel(channel);
    }
    delay(1);  // critical processing timeslice for NONOS SDK! No delay(0) yield()
  }

    while(1) {
        delay(1000);  // critical processing timeslice for NONOS SDK! No delay(0) yield()

        // dump and reset every second
        Serial.printf("----\r\n");
        for( int i = 0; i < cache_entry_size; i++ ) {
          // src mac
          for (int j = 0; j < 6; j++) Serial.printf("%02x", cache_entries[i].mac[j]);
          Serial.printf(" %d %d\r\n", cache_entries[i].rssi_total / cache_entries[i].total, cache_entries[i].total);
        }
        cache_entry_size = 0;
    }
}

