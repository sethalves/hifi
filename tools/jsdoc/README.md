#JavaScript Documentation Generation

##Prerequisites

* Install node.js.
* Install jsdoc via npm.  `npm install jsdoc -g`

If you would like the extra functionality for gravPrep:
* Run npm install

To generate html documentation for the High Fidelity JavaScript API:

* `cd tools/jsdoc`
* `jsdoc root.js -c config.json`

The out folder should contain index.html.

To generate the grav automation files, run node gravPrep.js after you have made a JSdoc output folder.

This will create files that are needed for hifi-grav and hifi-grav-content repos

The md files for hifi-grav-content are located in out/grav/06.api-reference.

The template twig html files for hifi-grav are located out/grav/templates.

if you would like to copy these to a local version of the docs on your system you can run with the follows args:

* node grav true "path/to/grav/" "path/to/grav/content"