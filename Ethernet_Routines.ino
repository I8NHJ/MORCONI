/*********************** getIpAddress *********************/
void getIpAddress()
{
  uint8_t St=0;
  // start the Ethernet connection:
  if (TeensyDebug){
    debugln("=== Waiting for IP address");
  }
  teensyMAC(MyMAC);  // Get MAC address from Teensy

  TimeIt = millis();
  if (StaticIP) {
    getFixedIpAddress();
    Ethernet.begin(MyMAC, MyIP, MyDNS, MyGateway, MyMask);
  }
  else {
    while (Ethernet.begin(MyMAC, 6000) == 0) {
      if (millis() - TimeIt > 10000)  { // Ten seconds to get a DHCP served address
        if (TeensyDebug) {
          debugln("Failed to configure Ethernet using DHCP");
          debug("link Status: "); debugln(Ethernet.linkStatus());
          debug("hardware Status: "); debugln(Ethernet.hardwareStatus());
          debug("socket Status: "); debugln(String(Ethernet.socketStatus(St)));
       }
      getFixedIpAddress();
      break;
      }
    }
  }

  if (TeensyDebug) {
    debug("link Status: "); debugln(Ethernet.linkStatus());
    debug("hardware Status: "); debugln(Ethernet.hardwareStatus());
    debug("socket Status: "); debugln(String(Ethernet.socketStatus(St)));
  }
  MyIP = Ethernet.localIP();
  MyGateway = Ethernet.gatewayIP();
  MyMask = Ethernet.subnetMask();
  MyDNS = Ethernet.dnsServerIP();
}

/*********************** getFixedIpAddress *********************/
void getFixedIpAddress() {
  MyIP = {CfgIP[0], CfgIP[1], CfgIP[2], CfgIP[3]};
  MyGateway = {CfgGateway[0], CfgGateway[1], CfgGateway[2], CfgGateway[3]};
  MyMask = {CfgMask[0], CfgMask[1], CfgMask[2], CfgMask[3]};
  MyDNS = {8,8,8,8};

  // start the Ethernet connection:
  if (TeensyDebug) {
    debugln("=== Setting fixed IP address");
  }
  if (Ethernet.linkStatus() == 1) {
    Ethernet.begin(MyMAC, MyIP, MyDNS, MyGateway, MyMask);
  }
}  // end getFixedIpAddress

/*********************** teensyMAC *********************/
void teensyMAC(uint8_t *MyMAC) {
  for (uint8_t by = 0; by < 2; by++) MyMAC[by] = (HW_OCOTP_MAC1 >> ((1 - by) * 8)) & 0xFF;
  for (uint8_t by = 0; by < 4; by++) MyMAC[by + 2] = (HW_OCOTP_MAC0 >> ((3 - by) * 8)) & 0xFF;
}
