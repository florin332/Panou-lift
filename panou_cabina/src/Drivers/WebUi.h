#ifndef WEB_UI_H
#define WEB_UI_H

namespace WebUi
{
    void begin();
    void poll();
    void captureServiceResponse(const char *response);
}

#endif