/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:

**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtWidgets>
#include "dialog.h"
//* login and createroom
#include "LoginDialog.h"
#include "RoomDialog.h"
//**********************
#include <time.h>
#include <curses.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>


#define MAX_MESSAGES 100
#define MAX_MESSAGE_LEN 300
#define MAX_RESPONSE (20 * 1024)

int lastMessage = 0;

int open_client_socket(char * host, int port) {
    // Initialize socket address structure
    struct  sockaddr_in socketAddress;

    // Clear sockaddr structure
    memset((char *)&socketAddress,0,sizeof(socketAddress));

    // Set family to Internet
    socketAddress.sin_family = AF_INET;

    // Set port
    socketAddress.sin_port = htons((u_short)port);

    // Get host table entry for this host
    struct  hostent  *ptrh = gethostbyname(host);
    if ( ptrh == NULL ) {
        perror("gethostbyname");
        exit(1);
    }

    // Copy the host ip address to socket address structure
    memcpy(&socketAddress.sin_addr, ptrh->h_addr, ptrh->h_length);

    // Get TCP transport protocol entry
    struct  protoent *ptrp = getprotobyname("tcp");
    if ( ptrp == NULL ) {
        perror("getprotobyname");
        exit(1);
    }

    // Create a tcp socket
    int sock = socket(PF_INET, SOCK_STREAM, ptrp->p_proto);
    if (sock < 0) {
        perror("socket");
        exit(1);
    }

    // Connect the socket to the specified server
    if (connect(sock, (struct sockaddr *)&socketAddress,
            sizeof(socketAddress)) < 0) {
        perror("connect");
        exit(1);
    }

    return sock;
}

int sendCommand(char * host, int port, char * command, char * user,
        char * password, char * args, char * response) {
    int sock = open_client_socket( host, port);

    // Send command
    write(sock, command, strlen(command));
    write(sock, " ", 1);
    write(sock, user, strlen(user));
    write(sock, " ", 1);
    write(sock, password, strlen(password));
    write(sock, " ", 1);
    write(sock, args, strlen(args));
    write(sock, "\r\n",2);

    // Keep reading until connection is closed or MAX_REPONSE
    int n = 0;
    int len = 0;
    while ((n=read(sock, response+len, MAX_RESPONSE - len))>0) {
        len += n;
    }
    response[len]=0;


    //printf("response:%s\n", response);

    close(sock);
}


//*******************************************************
char * toString(QString s) {
        QByteArray ba;
        ba = s.toLocal8Bit();
        char * str = ba.data();
        return str;
}

void Dialog::sendAction()
{
    QString str = inputMessage->toPlainText();
    char * message = strdup(str.toStdString().c_str());
    inputMessage->clear();
    char response[MAX_RESPONSE];
    char arg[50];
    sprintf(arg,"%s %s",room,message);
    sendCommand("localhost",2912,"SEND-MESSAGE",user,password,arg,response);




    /*QString str = inputMessage->toPlainText();
    QByteArray ba;
    ba = str.toLocal8Bit();
    char * message = ba.data();
    allMessages->append(message);
    inputMessage->clear();
    char response[MAX_RESPONSE];
    sendCommand("localhost", 2912, "ADD-USER", "user", "password", "", response);
    allMessages->append(response);
    printf("Send Button\n");*/
}

void Dialog::newUserAction()
{
    LoginDialog* loginDialog = new LoginDialog( this );
    loginDialog->exec();
    userx = loginDialog->getUsername();
    passwordx = loginDialog->getPassword();
    //*use
    /*QByteArray ba;
    ba = userx.toLocal8Bit();*/
    //user = strdup(ba.data());
    user = strdup(userx.toStdString().c_str());
    /*QByteArray ba1;
    ba1 = passwordx.toLocal8Bit();*/
    password = strdup(passwordx.toStdString().c_str());
    //***
    room = "main";
    char response[MAX_RESPONSE];
    sendCommand("localhost", 2912, "ADD-USER", user, password, "", response);
    /*sendCommand("localhost",2912,"GET-ALL-USERS", user, password, "", response);
    usersList->clear();
    usersList->addItem(response);
    /*user = "taeyeon";
    password = "beauty";
    room = strdup("main");
    char response[MAX_RESPONSE];
    sendCommand("localhost", 2912, "ADD-USER", user,password, "", response);
    if (!strcmp(response,"OK\r\n")) {
           usersList->addItem(user);
    }
    printf("New User Button\n");*/
}

void Dialog::newRoomAction()
{
     room = strdup(myRoomNameLineEdit->text().toStdString().c_str());
     //myRoomNameLineEdit->setHidden(true);
     myRoomNameLineEdit->clear();
     /*QByteArray ba;
     ba = userx.toLatin1();
     user = ba.data();
     QByteArray ba1;
     ba1 = passwordx.toLatin1();
     password = ba1.data();*/
    char response[MAX_RESPONSE];
    char message[50];
    sprintf(message,"%s Welcome to my chat room: %s!",room,room);
    sendCommand("localhost", 2912, "CREATE-ROOM", user, password,room, response);
    sendCommand("localhost", 2912, "ENTER-ROOM", user, password, room, response);
    sendCommand("localhost", 2912, "SEND-MESSAGE", user, password, message, response);
    sendCommand("localhost", 2912, "LEAVE-ROOM", user, password, room, response);

    room = strdup("main");

}

void Dialog::enterRoomAction(QListWidgetItem * item)
{
    //
    //**
    QString str = item->text();
    QByteArray ba3;
    ba3 = str.toLocal8Bit();
    char * roomname = ba3.data();
    room = strdup(roomname);
    char arg[50];
    sprintf(arg,"-1 %s",room);
    char response[MAX_RESPONSE];
    char message[50];
    sprintf(message,"%s entered the room",room);
    sendCommand("localhost", 2912, "ENTER-ROOM", user, password, room, response);
    sendCommand("localhost", 2912, "SEND-MESSAGE", user, password, message, response);
    /*(sendCommand("localhost", 2912, "GET-MESSAGES", user, password, arg, response);
    allMessages->clear();
    allMessages->append(response);*/


}

void Dialog::leaveRoomAction()
{
    char response[MAX_RESPONSE];
    char message[50];
    sprintf(message,"%s leave the room",room);
    sendCommand("localhost", 2912, "SEND-MESSAGE", user, password, message, response);
    sendCommand("localhost", 2912, "LEAVE-ROOM", user, password, room, response);
    room = strdup("main");
    allMessages->clear();

}

void Dialog::timerAction()
{
    printf("Timer wakeup\n");
    messageCount++;
    /*QByteArray ba;
    ba = userx.toLocal8Bit();
    user = ba.data();
    QByteArray ba1;
    ba1 = passwordx.toLocal8Bit();
    password = ba1.data();*/
    if (!strcmp(room,"no")) return;
    if (!strcmp(room,"main")) {
        allMessages->clear();
        allMessages->append("Welcome to the main lobby!");
        char response[MAX_RESPONSE];
        sendCommand("localhost", 2912, "GET-ALL-USERS", user, password, "", response);
        char *u;
        u = strtok(response, "\r\n");
        usersList->clear();
        while (u != NULL) {
            usersList->addItem(u);
            u= strtok(NULL, "\r\n");
        }
        //usersList->clear();
        //usersList->addItem(response);
        char response2[MAX_RESPONSE];
        sendCommand("localhost", 2912, "LIST-ROOMS", user, password, "", response2);
        if (strcmp(response2,"DENIED") == 0) {
            return;
         }
         else {
            char *pch;
            pch = strtok(response2, " ");
            roomsList->clear();
            while (pch != NULL) {
                roomsList->addItem(pch);
                pch = strtok(NULL, " ");
            }
         }

    }
    if(strcmp(room,"main") != 0){
        roomsList->clear();
        roomsList->addItem(room);
        //users in room
        char r1[MAX_RESPONSE];
        sendCommand("localhost", 2912, "GET-USERS-IN-ROOM", user, password, room, r1);
        usersList->clear();
        usersList->addItem(r1);
        //messages in room
        char r2[MAX_RESPONSE];
        char arg[50];
        sprintf(arg,"-1 %s",strdup(room));
        sendCommand("localhost", 2912, "GET-MESSAGES", user, password, arg, r2);
        allMessages->clear();
        allMessages->append(r2);
        }
    /*char message[50];
    sprintf(message,"Timer Refresh New message %d",messageCount);
    allMessages->append(message);*/
    /*if(user == NULL) {
        char response[MAX_RESPONSE];
        sendCommand("localhost", 2912, "GET-ALL-USERS", "taeyeon", "beauty", "", response);
        usersList->clear();
        usersList->addItem(response);
    }
    else if (!strcmp(room,"main") && user != NULL) {
           char response[MAX_RESPONSE];
           sendCommand("localhost", 2912, "GET-ALL-USERS", user, password, "", response);
           usersList->clear();
           usersList->addItem(response);
    }*/

}

Dialog::Dialog()
{
    createMenu();

    QVBoxLayout *mainLayout = new QVBoxLayout;

    // Rooms List
    QVBoxLayout * roomsLayout = new QVBoxLayout();
    QLabel * roomsLabel = new QLabel("Rooms");
    roomsList = new QListWidget();
    roomsLayout->addWidget(roomsLabel);
    roomsLayout->addWidget(roomsList);

    // Users List
    QVBoxLayout * usersLayout = new QVBoxLayout();
    QLabel * usersLabel = new QLabel("Users");
    usersList = new QListWidget();
    usersLayout->addWidget(usersLabel);
    usersLayout->addWidget(usersList);

    // Layout for rooms and users
    QHBoxLayout *layoutRoomsUsers = new QHBoxLayout;
    layoutRoomsUsers->addLayout(roomsLayout);
    layoutRoomsUsers->addLayout(usersLayout);

    // Textbox for all messages
    QVBoxLayout * allMessagesLayout = new QVBoxLayout();
    QLabel * allMessagesLabel = new QLabel("Messages");
    allMessages = new QTextEdit;
    allMessagesLayout->addWidget(allMessagesLabel);
    allMessagesLayout->addWidget(allMessages);

    // Textbox for input message
    QVBoxLayout * inputMessagesLayout = new QVBoxLayout();
    QLabel * inputMessagesLabel = new QLabel("Type your message:");
    inputMessage = new QTextEdit;
    inputMessagesLayout->addWidget(inputMessagesLabel);
    inputMessagesLayout->addWidget(inputMessage);

    // Send and new account buttons
    QHBoxLayout *layoutButtons = new QHBoxLayout;
    QPushButton * sendButton = new QPushButton("Send");
    QPushButton * newUserButton = new QPushButton("New Account/Login");
    QPushButton * newRoomButton = new QPushButton("Create Room");
    QPushButton * leaveRoomButton = new QPushButton("Leave Room");


    myRoomNameLineEdit = new QLineEdit;
    layoutButtons->addWidget(sendButton);
    layoutButtons->addWidget(newUserButton);
    layoutButtons->addWidget(leaveRoomButton);
    layoutButtons->addWidget(newRoomButton);

    layoutButtons->addWidget(myRoomNameLineEdit);



    // Setup actions for buttons
    connect(sendButton, SIGNAL (released()), this, SLOT (sendAction()));
    connect(newUserButton, SIGNAL (released()), this, SLOT (newUserAction()));
    //*enter room
    connect(roomsList,SIGNAL(itemDoubleClicked(QListWidgetItem *)),this,SLOT(enterRoomAction(QListWidgetItem *)));
    // create room button
    connect(newRoomButton, SIGNAL (released()), this, SLOT (newRoomAction()));
    connect(leaveRoomButton,SIGNAL(released()),this,SLOT(leaveRoomAction()));

    // Add all widgets to window
    mainLayout->addLayout(layoutRoomsUsers);
    mainLayout->addLayout(allMessagesLayout);
    mainLayout->addLayout(inputMessagesLayout);
    mainLayout->addLayout(layoutButtons);
    // Populate rooms
   /*for (int i = 0; i < 20; i++) {
        char s[50];
        sprintf(s,"Room %d", i);
        roomsList->addItem(s);
    }

    // Populate users
    *for (int i = 0; i < 20; i++) {
        char s[50];
        sprintf(s,"User %d", i);
        usersList->addItem(s);
    }*/

    /*for (int i = 0; i < 20; i++) {
        char s[50];
        sprintf(s,"Message %d", i);
        allMessages->append(s);
    }*/

    // Add layout to main window
    setLayout(mainLayout);

    setWindowTitle(tr("CS240 IRC Client"));
    //timer->setInterval(5000);

    messageCount = 0;

    timer = new QTimer(this);
    connect(timer, SIGNAL (timeout()), this, SLOT (timerAction()));
    timer->start(1000);
}


void Dialog::createMenu()

{
    menuBar = new QMenuBar;
    fileMenu = new QMenu(tr("&File"), this);
    exitAction = fileMenu->addAction(tr("E&xit"));
    menuBar->addMenu(fileMenu);

    connect(exitAction, SIGNAL(triggered()), this, SLOT(accept()));
}
