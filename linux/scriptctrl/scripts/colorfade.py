#!/usr/bin/python3
# -*- coding: utf-8 -*-

import traceback
import random
import sys
import colorsys
import time

mytrigger_ = "continue"
hsvvalue_="random"
fade_duration_=-1

def activate(scr, newsettings):
    global hsvvalue_, fade_duration_
    if "value" in newsettings and isinstance(newsettings["value"],(int,float)) and newsettings["value"] >= 0.0 and newsettings["value"] <= 1.0:
        hsvvalue_ = newsettings["value"]
    else:
        hsvvalue_ = "random"
    if "fadeduration" in newsettings and isinstance(newsettings["fadeduration"], int):
        fade_duration_= min(120000,max(900,newsettings["fadeduration"]))
    else:
        fade_duration_ = -1
    animateAllLights(scr)

def deactivate(scr):
    pass

def animateSomeLightsOnce(scr, targets, duration=1000, triggerme=False):
    if len(targets) == 0:
        return
    triggercc=[]
    if triggerme:
        triggercc=[mytrigger_]
    r,g,b = colorsys.hsv_to_rgb(random.randint(0,1000)/1000.0,1,random.randint(300,1000)/1000.0 if hsvvalue_ == "random" else hsvvalue_)
    r *= 1000.0
    g *= 1000.0
    b *= 1000.0
    r = int(min(r,1000))
    g = int(min(g*4.0/5.0,800))
    b = int(min(b*2.0/3.0,666))
    scr.setLight(targets[0],r=r,g=g,b=b,cw=0,ww=0,fade_duration=duration,cc=targets[1:],trigger_on_complete=triggercc)

def animateAllLights(scr):
    lst = scr.participating
    if len(lst) == 0:
        return
    random.shuffle(lst)
    lsthalf = int(len(lst)/2)
    duration = 9000
    if fade_duration_ >= 900 and fade_duration_ <= 120000:
        duration = fade_duration_
    else:
        duration=random.randint(6,50)*100
    animateSomeLightsOnce(scr,lst[:lsthalf],duration)
    animateSomeLightsOnce(scr,lst[lsthalf:],duration,True)

def init(scr):
    scr.registerActivate(activate)
    scr.registerDeactivate(deactivate)
    #scr.registerLoop(loop)
    scr.setDefaultParticipating(scr.lightidsceiling)
    scr.registerTrigger(mytrigger_, animateAllLights)
