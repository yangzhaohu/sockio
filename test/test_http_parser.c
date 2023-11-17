#include <stdio.h>
#include <string.h>
#include "proto/sio_httpprot.h"

const char *g_body = "HTTP/1.1 200 OK\r\n"
         "Date: Tue, 04 Aug 2009 07:59:32 GMT\r\n"
         "Server: Apache\r\n"
         "X-Powered-By: Servlet/2.5 JSP/2.1\r\n"
         "Content-Type: text/xml; charset=utf-8\r\n"
         "Connection: close\r\n"
         "\r\n"
         "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
         "<SOAP-ENV:Envelope xmlns:SOAP-ENV=\"http://schemas.xmlsoap.org/soap/envelope/\">\n"
         "  <SOAP-ENV:Body>\n"
         "    <SOAP-ENV:Fault>\n"
         "       <faultcode>SOAP-ENV:Client</faultcode>\n"
         "       <faultstring>Client Error</faultstring>\n"
         "    </SOAP-ENV:Fault>\n"
         "  </SOAP-ENV:Body>\n"
         "</SOAP-ENV:Envelope>";

const char *g_body1 = "HTTP/1.1 200 OK\r\n"
         "Date: Tue, 04 Aug 2009 07:59:32 GMT\r\n"
         "Server: Apache\r\n"
         "X-Powered-By: Servlet/2.5 JSP/2.1\r\n"
         "Content-Type: text/xml; ";
const char *g_body2 = "charset=utf-8\r\nConnection: close\r\n"
         "\r\n"
         "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
         "<SOAP-ENV:Envelope xmlns:SOAP-ENV=\"http://schemas.xmlsoap.org/soap/envelope/\">\n"
         "  <SOAP-ENV:Body>\n"
         "    <SOAP-ENV:Fault>\n"
         "       <faultcode>SOAP-ENV:Client</faultcode>\n"
         "       <faultstring>Client Error</faultstring>\n"
         "    </SOAP-ENV:Fault>\n"
         "  </SOAP-ENV:Body>\n"
         "</SOAP-ENV:Envelope>";

static char *g_resp =
            "HTTP/1.1 200 OK\r\n"
            "Connection: keep-alive\r\n"
            "Content-Type: text/html\r\n"
            "Content-Length: 125\r\n\r\n"
            "<html>"
            "<head><title>Hello</title></head>"
            "<body>"
            "<center><h1>Hello, Client</h1></center>"
            "<hr><center>SOCKIO</center>"
            "</body>"
            "</html>";
         
int main()
{
    struct sio_httpprot *http = sio_httpprot_create(SIO_HTTP_RESPONSE);
    int ret = sio_httpprot_process(http, g_resp, 96);
    SIO_LOGI("g_resp len: %d, ret: %d\n", (int)strlen(g_resp), ret);
    ret = sio_httpprot_process(http, g_resp + 96, strlen(g_resp) - 96);
    SIO_LOGI("g_resp2 len: %d, ret: %d\n", (int)strlen(g_resp), ret);

    getc(stdin);

    SIO_LOGI("-----------------------\r\n\r\n");
    struct sio_httpprot *http2 = sio_httpprot_create(SIO_HTTP_RESPONSE);
    sio_httpprot_process(http2, g_body1, strlen(g_body1));
    getc(stdin);
    sio_httpprot_process(http2, g_body2, strlen(g_body2));
    getc(stdin);

    return 0;
}
