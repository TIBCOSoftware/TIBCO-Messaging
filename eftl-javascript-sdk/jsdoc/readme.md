# How to build the doc

The eFTL javascript library has javadoc style comments using [JSDoc3](http://usejsdoc.org). To install JSDoc you'll need to install [Node.js](http://nodejs.org/).

To install (after having installed Node and npm):

* `npm install jsdoc`
* ./node_modules/jsdoc/jsdoc.js eftl.js -c jsdoc/conf.json jsdoc/eftl_package.md -t tibcotemplate
* Output of the doc should be in the `out` directory.
