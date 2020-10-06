// #include <ESP8266WiFi.h>
#include "time.h"
#include "telnet.h"
#include <memory>
using namespace std;


static const int nTelnetClients = 2;
static WiFiServer server(23);                    // --> default port for communication usign TELNET protocol | Server Instance
static WiFiClient serverClients[nTelnetClients]; // --> Client Instanse



void telnetStart()
{
    server.begin();
    server.setNoDelay(true); // --> Won't be storing data into buffer and wait for the ack. rather send the next data and in case nack is received, it will resend the whole data

    Serial.print("Ready! Use 'telnet ");
    Serial.print(WiFi.localIP());
    Serial.println(":23' to connect");
}

void telnetPrint(String message)
{
    Serial.print(message);
    unsigned int len = message.length() + 1; // one extra added for null char
    char msg[len];
    message.toCharArray(msg, len);
    for (int i = 0; i < nTelnetClients; i++)
    {
        if (serverClients[i] && serverClients[i].connected())
        {
            serverClients[i].write(msg, len);
        }
    }
}

void telnetPrintln(String message)
{
    Serial.println(message);
    const char *enterkeyCode = "\n\r";
    unsigned int len = message.length() + 1; // one extra added for null char.
    char msg[len];
    message.toCharArray(msg, len);
    for (int i = 0; i < nTelnetClients; i++)
    {
        if (serverClients[i] && serverClients[i].connected())
        {
            serverClients[i].write(currentTimeStamp().get());
            serverClients[i].write(msg, len);
            serverClients[i].write(enterkeyCode, sizeof(enterkeyCode) - 1); // one char removed to prevent the printing of null char.
        }
    }
}

void telnetPrintTime()
{
    for (int i = 0; i < nTelnetClients; i++)
    {
        if (serverClients[i] && serverClients[i].connected())
            serverClients[i].write(currentTimeStamp().get());
    }
}

void telnetPrintEnter()
{
    const char *enterkeyCode = "\n\r";
    for (int i = 0; i < nTelnetClients; i++)
        if (serverClients[i] && serverClients[i].connected())
            serverClients[i].write(enterkeyCode, sizeof(enterkeyCode) - 1); // one char removed to prevent the printing of null char.
}

void connectToClients()
{
    //check if there are any new clients
    if (server.hasClient())
    {
        for (int i = 0; i < nTelnetClients; i++)
        {
            //find free/disconnected spot
            if (!serverClients[i] || !serverClients[i].connected())
            {
                if (serverClients[i])
                    serverClients[i].stop();
                serverClients[i] = server.available();
                telnetPrintTime();
                telnetPrint("Client: ");
                telnetPrint((String)i);
                telnetPrint(" connected.");
                telnetPrintEnter();
                break;
            }
        }
        //no free/disconnected spot so reject
        WiFiClient serverClient = server.available();
        serverClient.stop();
    }
}

void receiveClientData()
{
    //check clients for data
    for (int i = 0; i < nTelnetClients; i++)
    {
        if (serverClients[i] && serverClients[i].connected())
        {
            if (serverClients[i].available())
            {
                //get data from the telnet client and push it to the UART
                while (serverClients[i].available())
                    Serial.write(serverClients[i].read());
            }
        }
    }
}

void sendDataFromUART()
{
    //check UART for data
    if (Serial.available())
    {
        unsigned int len = Serial.available();
        byte sbuffer[len];
        Serial.readBytes(sbuffer, len);
        //push UART data to all connected telnet clients
        for (int i = 0; i < nTelnetClients; i++)
        {
            if (serverClients[i] && serverClients[i].connected())
            {
                serverClients[i].write(sbuffer, len);
            }
        }
    }
}

void telnetHandler()
{
    connectToClients();
    receiveClientData();
    sendDataFromUART();
}