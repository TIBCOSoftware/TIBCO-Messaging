# How to build the JavaScript doc

The eFTL javascript library has javadoc style comments using [JSDoc3](http://usejsdoc.org). To install JSDoc you'll need to install [Node.js](http://nodejs.org/).

First install Node.js on your local machine or use one of the following:

* /rv/tools/nodejs/linux/
* /rv/tools/nodejs/mac/
* /rv/tools/nodejs/win/

To install and run JSDoc:

* Change to the cetus/eftl/clients/javascript directory.
* Install JSDoc by running `npm install jsdoc`.
* Update the eFTL version information in jsdoc/eftl_package.md.
* Run JSDoc with `./node_modules/jsdoc/jsdoc.js eftl.js -c jsdoc/conf.json jsdoc/eftl_package.md -t tibcotemplate -d doc/javascript`.
* Output will be sent to the `doc` directory. 
