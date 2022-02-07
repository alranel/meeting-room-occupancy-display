#undef PORT

struct ParsedURL {
  String protocol = "";
  String host = "";
  String port = "";
  String path = "";
};

enum URLParseState {PROTOCOL, SEPERATOR, HOST, PORT, PATH};

void parseURL(String urlString, ParsedURL* url) {
  // Assume a valid URL

  URLParseState state = PROTOCOL;

  url->protocol = "";
  url->host = "";
  url->port = "";
  url->path = "/";


  for (int i = 0; i < urlString.length(); i++) {
    switch(state)
    {
      case PROTOCOL: if (urlString[i] == ':') state = SEPERATOR;
                     else url->protocol += urlString[i];
                     break;
      case SEPERATOR: if (urlString[i] != '/') {
                          state = HOST;
                          url->host += urlString[i];
                      }
                      break;
      case HOST: if (urlString[i] == ':') state = PORT;
                 else {
                   if (urlString[i] == '/') state = PATH;
                   else url->host += urlString[i];
                   }
                 break;
      case PORT: if (urlString[i] == '/') state = PATH;
                 else  url->port += urlString[i];
                 break;
      case PATH: url->path += urlString[i];

    }
  }

  if (url->port == "") {
    if (url->protocol == "http") {
      url->port = "80";
    } else if (url->protocol == "https") {
      url->port = "443";
    }
  }
}
