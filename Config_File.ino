/***************************** GetConfigFile ***************************/
/* Those are the variables managed in the config file:
  STARTUP DELAY       default 250 (not in use)
  DEBOUNCE            default 0
  SIDETONE            default ON
  SIDETONE FREQ       default 600.0
  SIDETONE VOLUME     default 50
  FLEXIP
  FLEXPORT
  FLEXDELAY           default 3000
  STATIC IP           default FALSE
  TEENSYDEBUG         default FALSE
  TEENSYIP
  TEENSYGATEWAY
  TEENSYMASK
*/

void getConfigFile() {
  if (!SD.begin(chipSelect)) {
    #ifdef CONFIG_DEBUG 
      debugln("SD initialization failed!");
    #endif
    return;
  }
  else {
    #ifdef CONFIG_DEBUG 
      debugln("SD initialization success.");
    #endif
  }

  ConfigFile = SD.open("MORCONI.cfg");
  if (ConfigFile) {
    #ifdef CONFIG_DEBUG 
      debugln("Reading MORCONI.cfg");
    #endif
    while (ConfigFile.available()) {
      Rchar = ConfigFile.read();
      InBuf += String(Rchar);
      if (Rchar == 10 || ConfigFile.available() <= 0) {
        #ifdef CONFIG_DEBUG
          debug("Config Line: "); debug(InBuf);
        #endif
        ParseInBuf();
        InBuf = "";
      }
    }
    ConfigFile.close();
  }
  else {
    #ifdef CONFIG_DEBUG
      debugln("Error opening MORCONI.cfg");
    #endif
  }
}

/***************************** ParseInBuf ***************************/
void ParseInBuf() {
  String tmpStr;
  InBuf          = (InBuf.trim()).toUpperCase();
  if (InBuf.indexOf(";") == 0) {
    return;
  }
  if (InBuf.indexOf(";") > 0) {
    InBuf = InBuf.substring(0, InBuf.indexOf(";"));
  }
  if (InBuf.indexOf("STARTUP DELAY:") >= 0 && InSetup) {
    tmpStr       = InBuf.substring(InBuf.indexOf("STARTUP DELAY:") + 15);
    StartUpDelay = tmpStr.toInt();
    #ifdef CONFIG_DEBUG
      debug("Parsed StartUpDelay: "); debugln(StartUpDelay);
    #endif
    return;
  }
  if (InBuf.indexOf("DEBOUNCE:") >= 0 && InSetup) {
    tmpStr      = InBuf.substring(InBuf.indexOf("DEBOUNCE:") + 10);
    Debounce = tmpStr.toInt();
    #ifdef CONFIG_DEBUG
      debug("Parsed Debounce: "); debugln(Debounce);
    #endif
    return;
  }
  if (InBuf.indexOf("SIDETONE:") >= 0 && InSetup) {
    SidetoneActive = InBuf.substring(InBuf.indexOf("SIDETONE:") + 10).trim() == "ON";
    #ifdef CONFIG_DEBUG
      debug("Parsed Sidetone: "); debugln(SidetoneActive);
    #endif
    return;
  }
  if (InBuf.indexOf("SIDETONE FREQ:") >= 0 && InSetup) {
    tmpStr = InBuf.substring(InBuf.indexOf("SIDETONE FREQ:") + 15).trim();
    SidetoneFrequency = tmpStr.toFloat();
    if (SidetoneFrequency < 100.0) {SidetoneFrequency = 100.0;}
    if (SidetoneFrequency > 2000.0) {SidetoneFrequency = 2000.0;}
    #ifdef CONFIG_DEBUG
      debug("Parsed SidetoneFrequency: "); debugln(SidetoneFrequency);
    #endif
    return;
  }
  if (InBuf.indexOf("SIDETONE VOLUME:") >= 0 && InSetup) {
    tmpStr = InBuf.substring(InBuf.indexOf("SIDETONE VOLUME:") + 17).trim();
    SidetoneVolume = tmpStr.toFloat()/100.0;
    if (SidetoneVolume < 0.0) {SidetoneVolume = 0.0;}
    if (SidetoneVolume > 1.0) {SidetoneVolume = 1.0;}
    #ifdef CONFIG_DEBUG
      debug("Parsed SidetoneVolume: "); debugln(SidetoneVolume);
    #endif
    return;
  }
  if (InBuf.indexOf("FLEXIP:") >= 0 && InSetup) {
    InBuf = InBuf.substring(InBuf.indexOf("FLEXIP:") + 8);
    ParseIP(InBuf, FlexIP);
    RadioIP = {FlexIP[0], FlexIP[1], FlexIP[2], FlexIP[3]};
    #ifdef CONFIG_DEBUG
      debugf4("Parsed FlexIP:  %u.%u.%u.%u\n", FlexIP[0],FlexIP[1],FlexIP[2],FlexIP[3]);
    #endif
    return;
  }
  if (InBuf.indexOf("FLEXPORT:") >= 0 && InSetup) {
    tmpStr = InBuf.substring(InBuf.indexOf("FLEXPORT:") + 10);
    FlexPort = tmpStr.toInt();
    #ifdef CONFIG_DEBUG
      debug("Parsed FlexPort: "); debugln(FlexPort);
    #endif
    return;
  }
  if (InBuf.indexOf("FLEXDELAY:") >= 0 && InSetup) {
    tmpStr       = InBuf.substring(InBuf.indexOf("FLEXDELAY:") + 11);
    StartUpDelay = tmpStr.toInt();
    #ifdef CONFIG_DEBUG
      debug("Parsed FlexDelay: "); debugln(FlexDelay);
    #endif
    return;
  }
  if (InBuf.indexOf("TEENSYDEBUG:") >= 0 && InSetup) {
    TeensyDebug = InBuf.substring(InBuf.indexOf("TEENSYDEBUG:") + 13).trim() == "TRUE";
    #ifdef CONFIG_DEBUG
      debug("Parsed Teensy Debug: "); debugln(TeensyDebug);
    #endif
    return;
  }
    if (InBuf.indexOf("STATIC IP:") >= 0 && InSetup) {
    StaticIP = InBuf.substring(InBuf.indexOf("STATIC IP:") + 11).trim() == "TRUE";
    #ifdef CONFIG_DEBUG
      debug("Parsed Static IP: "); debugln(StaticIP);
    #endif
    return;
  }
  if (InBuf.indexOf("TEENSYIP:") >= 0 && InSetup) {
    InBuf = InBuf.substring(InBuf.indexOf("TEENSYIP:") + 10);
    ParseIP(InBuf, CfgIP);
    #ifdef CONFIG_DEBUG
      debugf4("Parsed CfgIP:  %u.%u.%u.%u\n", CfgIP[0], CfgIP[1], CfgIP[2], CfgIP[3]);
    #endif
    return;
  }
  if (InBuf.indexOf("TEENSYGATEWAY:") >= 0 && InSetup) {
    InBuf = InBuf.substring(InBuf.indexOf("TEENSYGATEWAY:") + 15);
    ParseIP(InBuf, CfgGateway);
    #ifdef CONFIG_DEBUG
      debugf4("Parsed MyGateway:  %u.%u.%u.%u\n", CfgGateway[0], CfgGateway[1], CfgGateway[2], CfgGateway[3]);
    #endif
    return;
  }
  if (InBuf.indexOf("TEENSYMASK:") >= 0 && InSetup) {
    InBuf = InBuf.substring(InBuf.indexOf("TEENSYMASK:") + 12);
    ParseIP(InBuf, CfgMask);
    #ifdef CONFIG_DEBUG
      debugf4("Parsed MyMask:  %u.%u.%u.%u\n", CfgMask[0], CfgMask[1], CfgMask[2], CfgMask[3]);
    #endif
    return;
  }
}

/***************************** ParseIP ***************************/
void ParseIP(String &IPstr, uint8_t IP[4]) {
  int DotIDX;
  String tmpStr;
  for (int i = 0; i < 4; i++) {
    if (IPstr.indexOf(".") >= 0) {
      DotIDX = IPstr.indexOf(".") + 1;
      tmpStr = IPstr.substring(0, DotIDX);
      IP[i]  = tmpStr.toInt();
      IPstr  = IPstr.substring(DotIDX);
    }
    else {
      IP[i] = IPstr.toInt();
    }
  }
}
