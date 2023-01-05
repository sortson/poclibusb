# poclibusb
A simple POC for the use of libusb to control a PS3 pad over a web browser using WASM.

To locally compile just use cmake.


To compile with Webassembly just do:
```shell
cd build
em++ ../src/application.cpp -o helloworld.html -I/home/user/library/include/ /home/user/library/lib/libusb-1.0.a --bind -s ASYNCIFY -s ALLOW_MEMORY_GROWTH -s INVOKE_RUN=0 -s EXPORTED_RUNTIME_METHODS=['callMain']; sudo cp helloworld.* .../www/
```

And later: 
```shell
serve 
```

To access the POC open in your browser http://localhost:3000/helloworld

And type in console:
```javascript 
navigator.usb.requestDevice({filters: []})
```
Select your PS3 pad and next: 
```javascript
Module.callMain()
```
