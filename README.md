# DrainQueueCongestion
Always drain buffer occupation in routers.  
Copy code from quic protocol for simulation purpose.  
TODO  
- [x] Stream  
- [x] Ack  
- [x] Stop waiting  
- [x] Send packet through socket  
- [x] congestion control   

As for ns3 test case, the wscript file gives clear hint how to arrange this file  
in the right position in ns3.  
And add the CPLUS_INCLUDE_PATH flag in /etc/profile, for example:  
```
export CPLUS_INCLUDE_PATH=CPLUS_INCLUDE_PATH:/home/zsy/C_Test/ns-allinone-3.26/ns-3.26/src/dqc/model/thirdparty/include:/home/zsy/C_Test/ns-allinone-3.26/ns-3.26/src/dqc/model/thirdparty/congestion/:/home/zsy/C_Test/ns-allinone-3.26/ns-3.26/src/dqc/model/thirdparty/logging
```
Run simulation:  
```
./waf --run "scratch/dqc-test --it=1"
```
simulation results:   
Test with 3 flow in a point to point channel(3Mbps, one way delay 100ms, max queue length 300ms).  
bandwidth fairness:  
![avatar](https://github.com/SoonyangZhang/DrainQueueCongestion/blob/master/result/bw.png)  
one way delay  
![avatar](https://github.com/SoonyangZhang/DrainQueueCongestion/blob/master/result/delay.png)  

And the pcc code is copied from https://github.com/PCCproject/PCC-Uspace/tree/master/src/pcc. But it does not work here for now.  

