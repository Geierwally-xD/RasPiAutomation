#!/bin/bash
#

case "$1" in
  0)
    gpio export 5 out
    gpio -g write 5 0
    gpio export 6 out
    gpio -g write 6 0
    gpio export 7 out
    gpio -g write 7 0
    gpio export 13 out
    gpio -g write 13 0
    gpio export 19 out
    gpio -g write 19 0
    gpio export 26 out
    gpio -g write 26 0
    gpio export 16 out
    gpio -g write 16 0
    gpio export 20 out
    gpio -g write 20 0
    gpio export 21 out
    gpio -g write 21 0
    sudo chmod 777 -c -R /dev/ttyUSB0
    stty -F /dev/ttyUSB0 9600 raw -echo  #CONFIGURE SERIAL PORT
    echo -n '(MX*:RES!)' > /dev/ttyUSB0  #SEND COMMAND STRING TO SERIAL PORT
    killall -SIGKILL RasPiAutomation
;;
   00)
    killall -SIGKILL RasPiAutomation
;;
   10)
    sudo chmod 777 -c -R remote-debugging
    sudo nice --15  remote-debugging/RasPiAutomation $1 $2 &

;;
   11)
    sudo chmod 777 -c -R remote-debugging
    sudo nice --15  remote-debugging/RasPiAutomation  $1 $2 &
;;
   20)
    sudo chmod 777 -c -R remote-debugging
    sudo nice --15  remote-debugging/RasPiAutomation $1 $2 &
;;
   21)
    sudo chmod 777 -c -R remote-debugging
    sudo nice --15  remote-debugging/RasPiAutomation  $1 $2 &
;;
   30)
    sudo chmod 777 -c -R remote-debugging
    sudo nice --15  remote-debugging/RasPiAutomation  $1 $2 &
;;
   31)
   sudo chmod 777 -c -R remote-debugging
   sudo nice --15  remote-debugging/RasPiAutomation   $1 $2 &
;;
   40)
   sudo  chmod 777 -c -R remote-debugging
   sudo nice --15   remote-debugging/RasPiAutomation  $1 $2 &
;;
   41)
   sudo chmod 777 -c -R remote-debugging
   sudo nice --15  remote-debugging/RasPiAutomation  $1 $2 &
;;
   42)
   sudo chmod 777 -c -R remote-debugging
   sudo nice --15 remote-debugging/RasPiAutomation  $1 $2 &
;;
   43)
   sudo chmod 777 -c -R remote-debugging
   sudo nice --15 remote-debugging/RasPiAutomation  $1 $2 &
;;
   44)
   sudo chmod 777 -c -R remote-debugging
   sudo nice --15 remote-debugging/RasPiAutomation  $1 $2 &
;;
   52)
   sudo chmod 777 -c -R remote-debugging
   sudo nice --15 remote-debugging/RasPiAutomation  $1 $2 &
;;
   60)
   sudo chmod 777 -c -R remote-debugging
   sudo nice --15 remote-debugging/RasPiAutomation  $1 $2 &
;;
   61)
   sudo chmod 777 -c -R remote-debugging
   sudo nice --15 remote-debugging/RasPiAutomation  $1 $2 &
;;
   62)
   sudo chmod 777 -c -R remote-debugging
   sudo nice --15 remote-debugging/RasPiAutomation  $1 $2 &
;;
   63)
   sudo chmod 777 -c -R remote-debugging
   sudo nice --15 remote-debugging/RasPiAutomation  $1 $2 &
;;
   50)
   sudo  chmod 777 -c -R remote-debugging
   sudo nice --15 remote-debugging/RasPiAutomation  $1 $2 &

   case $2 in
     15)
        sudo shutdown -h now
     ;;
     *)
     ;;
    esac
;;  
   *)
    echo "Ivalid parameter!!! Values 0 - 19 are valid"
;;
esac
