Web development as a breeze

breeze is fast, lightweight, powerful, very easy to use web framework that uses lua as a scripting language embedded in C++ server. 
Server uses libevent for asynchronous and efficient network communication, but requests are executed in thread pool that allows to execute requests synchronously. 

Features:

 - high performance
     
 - support for rapid development
     
 - seamless JSON support

 - OOP architecture gives great flexibility and extensibility



**Installing**

    git clone git://github.com/sergeyzavadski/breeze.git
    cd breeze
    ./configure
    sudo make install
    
**Using**    

edit and save file helloworld.lua:
    
    require 'breeze'

    HelloWorldHandler = class('HelloWorldHandler', Handler)

    function HelloWorldHandler:get()
        response.body = {hello='world'}
    end 


    breeze.addHandler{path='/hello', handler=HelloWorldHandler}
    
run
    
    breeze helloworld.lua
    
Now there is a webserver running that responds with text 'Hello world' on url http://localhost:8080/hello. To test

    curl http://localhost:8080/hello
    
response is

    {"hello":"world"}
    
    
Please see [wiki](https://github.com/sergeyzavadski/breeze/wiki) for more information    

    

