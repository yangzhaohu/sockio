#include <stdio.h>
#include <string.h>
#include "proto/sio_http.h"

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
         
int main()
{
    struct sio_http *http = sio_http_create(SIO_HTTP_RESPONSE);
    sio_http_append_data(http, g_body, strlen(g_body));

    getc(stdin);

    printf("-----------------------\r\n\r\n");
    struct sio_http *http2 = sio_http_create(SIO_HTTP_RESPONSE);
    sio_http_append_data(http2, g_body1, strlen(g_body1));
    getc(stdin);
    sio_http_append_data(http2, g_body2, strlen(g_body2));
    getc(stdin);

    return 0;
}
