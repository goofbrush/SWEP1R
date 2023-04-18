#pragma once

#include "WinMem.h"
#include <list>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <unistd.h>


// inRace = exe + 0xA9BB81
using BYTE = unsigned char;
#define PBSIZE 0x310 // 0x30C+0x4
#define CBSIZE 0x12C // 0x128+0x4

class Node {
public:
    Node(BYTE* p, BYTE* c, float t) {
        playerBlock = p; cameraBlock = c; timer = t;
    }

    void print() {
        std::cout << timer << std::endl;
    }

    BYTE* playerBlock;
    BYTE* cameraBlock;
    // BYTE* trackBlock;
    float timer;
};

class Record {

public:

    BYTE track;
    BYTE racer;

    std::list<Node>::iterator getFront() { return data.begin(); }
    Node* getFirstNode() { return &data.back(); }
    Node* getLastNode() { return &data.back(); }
    int getSize() { return data.size(); }

    void addNode(BYTE* p, BYTE* c, float t) {
        data.push_back( Node(p,c,t) );
    }

    float getPreviousTimer() {
        if (data.empty()) return -1;
        return data.back().timer;
    }

    
    void saveRecording(const char* fName) {
        std::fstream file2(fName, std::fstream::out | std::fstream::binary);
        for (auto const& node : data) {
            file2.write(reinterpret_cast<const char*>(node.playerBlock), PBSIZE);
            file2.write(reinterpret_cast<const char*>(node.cameraBlock), CBSIZE);
            file2.write(reinterpret_cast<const char*>(&node.timer), 0x4);
        }
        file2.close();
    }

    void readFile(const char* fileLocation) {
        data.clear();
        std::fstream file(fileLocation, std::fstream::in | std::fstream::binary | std::ios::ate);
        
        int fileSize = file.tellg();
        int blockSize = PBSIZE + CBSIZE + 0x4;

        file.seekg(0, std::ios::beg);

        for (int i=0; i < fileSize; i+=blockSize) {
            unsigned char* player = new unsigned char[PBSIZE];
            unsigned char* camera = new unsigned char[CBSIZE];
            float timer;

            file.read(reinterpret_cast<char*>(player), PBSIZE);
            file.read(reinterpret_cast<char*>(camera), CBSIZE);
            file.read(reinterpret_cast<char*>(&timer), 0x4);

            addNode(player,camera,timer);
        }
        file.close();
    }

private:
    std::list<Node> data;
};



class Racer {

public:
    Record rec, play;

    bool inRace() { return mem.readValue<bool>(0xA9BB81, false, exe); }
    float getTimer() { return mem.readValue<float>(0xD78A4, true, exe, 0x74); }
    BYTE getTrack() { return mem.readValue<BYTE>(0xEA02B0, false, ""); }
    BYTE getRacer() { return mem.readValue<BYTE>(0xBFDB8, true, exe, 0x73); }
    bool isTabbedOut() { return mem.readValue<bool>(0x10CB64, false, exe); }

    BYTE* getPlayerBlock() { return mem.readMemory(PBSIZE, 0xA29C44, true, exe); }
    BYTE* getCameraBlock() { return mem.readMemory(CBSIZE, 0xA9ADAC, true, exe); }
    bool setPlayerBlock(const unsigned char* data) { return mem.writeMemory(data,PBSIZE,0xA29C44,true,exe); }

    bool isConnected() { return mem.isValid(); }
    
    bool getMode() { return recMode; }
    void setMode(bool isRecording) { recMode = isRecording; playMode = !isRecording; }


protected:

    virtual void updateUI(const char* flag) { (void)flag; } // overriden by the QT wrapper
    void stop() { m_running = false; }
    
    void init()  {
        exe = mem.getExeName();
        connected = mem.isValid();
        updateUI("connection");
        rec = Record();
        raceFlag = false;
    }
    
    void update() {
        connected = testConnection();
        if(!connected) { usleep(1000000); return; }

        if(inRace()) {

            if(recMode) {
                if(!raceFlag) {
                    rec.track = getTrack();
                    rec.racer = getRacer();
                    raceFlag = true;
                    updateUI("mode");
                }

                float timer = getTimer();

                if(timer > rec.getPreviousTimer()) {
                    BYTE* playerBlock = getPlayerBlock();
                    BYTE* cameraBlock = getCameraBlock();

                    rec.addNode( playerBlock, cameraBlock, timer );
                    rec.getLastNode()->print();

                    updateUI("nextFrame");
                }
            }
            else if(playMode) {
                if(!raceFlag) {
                    play.readFile(playFile);
                    current = play.getFront();
                    playTimer = 0;
                    updateUI("mode");
                }

                float timer = getTimer();

                while(timer > current->timer) {
                    current++;
                }

                if(timer > playTimer) {
                    setPlayerBlock(current->playerBlock);
                    playTimer = timer;
                    // updateUI("nextFrame");
                }

                
            }
            
        }
        else if(raceFlag == true) {
            const char* t = "test.bin";
            if(recMode) { rec.saveRecording(t); }

            rec = Record();
            play = Record();
            raceFlag = false;
            
            updateUI("mode");
        }
    }
    
private:
    const char* exe = "SWEP1RCR.EXE";
    Memory mem = Memory("Episode I Racer");
    bool connected = false;

    
    std::list<Node>::iterator current;
    bool raceFlag = false;
    bool m_running = true;
    bool recMode = true;

    bool playMode = false;
    float playTimer = 0;
    const char* playFile = "test.bin";

    bool testConnection() {
        if(!mem.isValid()) {
            if (connected) { updateUI("connection"); printf("Game Closed\n"); }

            printf("Attempting to Connect ...\n");
            bool isConnected = mem.connect("Episode I Racer");
            if(isConnected) { updateUI("connection"); exe = mem.getExeName(); } 
            return isConnected;
        }
        return true;
    }
    
};
