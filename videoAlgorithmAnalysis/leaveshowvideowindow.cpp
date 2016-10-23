﻿#pragma execution_character_set("utf-8")
#include "leaveshowvideowindow.h"
#include <iostream>
#include <QDebug>
#include <QMessageBox>
#include <QPainter>
#include <QPen>
#include <QPolygon>
#include <QDir>

LeaveShowVideoWindow::LeaveShowVideoWindow(QWidget *parent) : QWidget(parent)
{
    initData();
    initConnect();
    std::cout<<"LeaveShowVideoWindow()"<<std::endl;
}

LeaveShowVideoWindow::~LeaveShowVideoWindow()
{
    isOpen=false;
    if(leaveDetectThread)
    {
        leaveDetectThread->stopThread();
        leaveDetectThread->wait();
        delete leaveDetectThread;
        leaveDetectThread=NULL;
    }
    if(videoSave)
    {
        delete videoSave;
        videoSave=NULL;
    }
    std::cout<<"~LeaveShowVideoWindow()"<<std::endl;
}

void LeaveShowVideoWindow::paintEvent(QPaintEvent *e)
{
    QWidget::paintEvent(e);
    QPainter painter(this);
    if(isOpen)
    {
        QPixmap tempPicture=QPixmap::fromImage(currentImage);
        drawingArea(tempPicture);
        painter.drawPixmap(0,0,tempPicture);
    }
    else
    {
        painter.drawPixmap(0,0,QPixmap::fromImage(QImage(":/img/play.png")));
    }
    painter.end();
}

void LeaveShowVideoWindow::slotVideoImage(QImage image,bool isOpen)
{
    this->isOpen=isOpen;
    currentImage=image;
    this->update();
    if(!isOpen)
    {
        emit signalVideoMessage(isOpen);
    }
}

void LeaveShowVideoWindow::slotMessage(QString message,int pos)
{
    QString str="Leave Detection:";
    qDebug()<<message;
    if(message.trimmed().startsWith(str))
    {
//        QString filePath = QDir::currentPath()+"/Resource/"+QDate::currentDate().toString("yyyy-MM-dd");
//        QString fileName = QDir::currentPath()+"/Resource/"+QDate::currentDate().toString("yyyy-MM-dd")+"/" + QTime::currentTime().toString("hhmmsszzz")+".avi";
//        videoSave->saveVideoData2(videoPath,filePath,fileName,pos/1000,maxLeaveTime);
        int number=message.trimmed().mid(str.length()).toInt();
        int stopPos=pos/1000;
        int startPos=stopPos-maxLeaveTime;
        if(startPos<0)
        {
            startPos=0;
        }
        emit signalLeaveMessage(number,startPos,stopPos);
    }
}

void LeaveShowVideoWindow::slotSaveVideo(QString path)
{
    qDebug()<<"path:"<<path;
}

int LeaveShowVideoWindow::showVideo(const QString &path)
{
    int errCode=0;
    videoPath = path;
    if(leaveDetectThread)
    {
        errCode=leaveDetectThread->startThread(path);
        if(errCode>=0)
        {
            QFileInfo info1("./config/LeaveDetection.xml");
            if(info1.exists())
            {
                loadLeaveConfig();
                leaveDetectThread->initData();//初始化离岗检测参数
            }
            leaveDetectThread->start();
            isOpen=true;
        }
        else
        {
            isOpen=false;
        }
    }
    return 0;
}
//关闭视频
int LeaveShowVideoWindow::closeShowVideo()
{
    if(leaveDetectThread)
    {
        leaveDetectThread->stopThread();
        isOpen=false;
    }
    return 0;
}

//是否打开视频
bool LeaveShowVideoWindow::getIsOpenVideo()
{
    return isOpen;
}

//配置检测参数
void LeaveShowVideoWindow::setConfigParameter(QList<QPolygonF> detectArea,int maxLeaveTime)
{
    if(detectArea.isEmpty())
    {
        qDebug()<<"区域为空"<<endl;
        return;
    }
    this->maxLeaveTime=maxLeaveTime;
    area.clear();
    copyArea=detectArea;
    for(int loop=0;loop<detectArea.size();loop++)
    {
        std::vector<cv::Point> tempPoint;
        QList<QPointF> tempData=detectArea[loop].toList();
        if(tempData.size()<=2)
        {
            qDebug()<<QString("区域%1不合法").arg(loop)<<endl;
            continue;
        }
        for(int loop1=0;loop1<tempData.size();loop1++)
        {
            cv::Point point((int)tempData[loop1].x(),(int)tempData[loop1].y());
            tempPoint.push_back(point);
        }
        area.push_back(tempPoint);
    }
    saveLeaveConfig();
    leaveDetectThread->initData();
}

//开始检测
void LeaveShowVideoWindow::startDetect()
{
    leaveDetectThread->startDetect();
}

//停止检测
void LeaveShowVideoWindow::stopDetect()
{
    leaveDetectThread->stopDetect();
}

//开始播放
void LeaveShowVideoWindow::startVideo()
{
    leaveDetectThread->startShowVideo();
}

//停止播放
void LeaveShowVideoWindow::stopVideo()
{
    leaveDetectThread->stopShowVideo();
}

//得到图片
QImage LeaveShowVideoWindow::getImage()
{
    return currentImage;
}

//绘制区域
void LeaveShowVideoWindow::drawingArea(QPixmap &pixmap)
{
    QPainter painter(&pixmap);
    painter.setPen(myPen);
    //设置圆滑边框
    painter.setRenderHint(QPainter::Antialiasing,true);
    for(int loop=0;loop<copyArea.size();loop++)
    {
        painter.drawPolygon(copyArea[loop]);
    }
    painter.end();
}

void LeaveShowVideoWindow::initData()
{

    area.clear();
    copyArea.clear();
    maxLeaveTime=5*60;//最大离岗时间，超过这个时间就报警

    videoPath="";

    leaveDetectThread=new LeaveDetectThread();//离岗检测
    videoSave=new VideoSave();//保存视频
    isOpen=false;

    currentImage=QImage(":/img/play.png");

    //绘图画笔
    myPen.setWidth(2);
    myPen.setStyle(Qt::SolidLine);
    myPen.setColor(Qt::green);
}

void LeaveShowVideoWindow::initConnect()
{
    connect(leaveDetectThread,&LeaveDetectThread::signalMessage,this,&LeaveShowVideoWindow::slotMessage);
    connect(leaveDetectThread,&LeaveDetectThread::signalVideoMessage,this,&LeaveShowVideoWindow::slotVideoImage);

    connect(videoSave,&VideoSave::signalSaveFinish,this,&LeaveShowVideoWindow::slotSaveVideo);
}

void LeaveShowVideoWindow::saveLeaveConfig()
{
    cv::FileStorage fs;
    QDir makeDir;
    if(!makeDir.exists("./config/"))
    {
        if(!makeDir.mkdir("./config/"))
        {
            std::cout<<"make dir fail!"<<std::endl;
            return;
        }
    }
    fs.open("./config/LeaveDetection.xml",cv::FileStorage::WRITE,"utf-8");

    cv::write(fs,"maxLeaveCount",-1);
    cv::write(fs,"maxLeaveTime",maxLeaveTime);
    cv::write(fs, "crossMatchMaxValue", 0.1);
    cv::write(fs, "minBox", 300);
    cv::write(fs,"savePictureDir","./leave");

    for(int loop=0;loop<(int)area.size();loop++)
    {
        QString tempName="pointsArea"+QString::number(loop);
        cv::write(fs,tempName.toStdString().c_str(),area[loop]);
    }
    fs.release();
}

void LeaveShowVideoWindow::loadLeaveConfig()
{
    cv::FileStorage fs;
    std::vector<cv::Point> tempVector;
    area.clear();
    copyArea.clear();
    fs.open("./config/LeaveDetection.xml",cv::FileStorage::READ,"utf-8");


    cv::read(fs["maxLeaveTime"],maxLeaveTime,5*60);

    cv::FileNode node=fs["pointsArea0"];
    if(node.isNone())
    {
        return;
    }
    cv::FileNodeIterator iterator=node.begin(),iterator_end=node.end();
    for(int loop=0;iterator!=iterator_end;iterator++,loop++)
    {
        QString tempName="pointsArea"+QString::number(loop);
        if(fs[tempName.toStdString().c_str()].isNone())
            break;
        cv::read(fs[tempName.toStdString().c_str()],tempVector);
        area.push_back(tempVector);
    }

    for(int loop=0;loop<(int)area.size();loop++)
    {
        QVector<QPoint> tempData;
        for(int loop1=0;loop1<(int)area[loop].size();loop1++)
        {
            cv::Point tempPoint=area[loop][loop1];
            tempData.push_back(QPoint(tempPoint.x,tempPoint.y));
        }
        QPolygon tempPolygon(tempData);
        copyArea.push_back(tempPolygon);
    }

    fs.release();
}