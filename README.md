php-pecl-fsevents
=================

A PECL extension that adds support for OS X's fsevents file change notification API in
PHP.


Why
---

It's often useful to detect filesystem changes efficiently in build tools.  If you can
detect changes in a specified filesystem path, you can automatically trigger test runs
and other quality checks to provide faster and more useful feedback to the developer.

This extension adds support for Mac OS X's fsevents API to support this use case.  PHP
has long had support for Linux's inotify (http://php.net/inotify), but support for the
equivalent API on the Mac has been missing.  

This may seem less important in an environment where many people are developing using 
tools like Vagrant in order to more closely mirror their production environment.  
However, VMs often complicate filesystem monitoring because their synced folder features 
bypass the Linux VFS layer, making inotify inoperable.  Having support for fsevents 
in your host OS allows you to bridge that gap by either running your QA tools on the 
Mac directly, or using fsevents triggers to sync up folders in the VM or proxy the 
change events into the VM.


Building the Extension
----------------------

To build the extension, execute the following commands and then copy the resulting
`modules/fsevents.so` file to your PHP modules folder and add it to your php.ini.

    $ phpize
    $ ./configure --enable-fsevents
    $ make

We plan to get the extension added to PECL proper and homebrew soon for easier
installation.


Using the Extension
-------------------

The extension currently provides two functions: `fsevents_add_watch` and
`fsevents_start`.  You call `fsevents_add_watch` for each path and callback
combination you want to monitor.  Once all the watches are added, call
`fsevents_start` to kick things off.  At that point, a CoreFoundation run
loop will begin running, harvesting filesystem events from fseventsd and 
triggering the correct callbacks for the modified folders.

    <?php

    fsevents_add_watch(
        '/Users/me/my-path',
        function (array $events) {
            echo "This function is called every time a file in the path is changed\n";
        }
    );

    fsevents_start();


Status
------

The module works well for simple use cases now.  We still need to add some info
to the events array that is passed to your callback functions.  We also plan to
add some functions to allow you to halt all watches or remove an individual watch.
