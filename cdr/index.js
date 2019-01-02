'use strict';

var path = require('path');

const CdrPlugin = require(path.join(global.CDR_INSTALL_DIR, '/commander/classes/CdrPlugin')).CdrPlugin;

module.exports = class CfeCdrPlugin extends CdrPlugin {
  constructor(urlBase) {
    super('HMC5883', path.join(__dirname, 'web', urlBase));
  }

  getContent() {
    var result = {
      shortDescription: 'HMC5883 Driver',
      longDescription: 'HMC5883 Driver',
      nodes: {
				main: {
					type: CdrPlugin.ContentType.LAYOUT,
					shortDescription: 'Main',
					longDescription: 'Main HMC5883.',
					filePath: '/main_layout.lyt'
				},
				hk: {
					type: CdrPlugin.ContentType.PANEL,
					shortDescription: 'HMC5883 HK',
					longDescription: 'HMC5883 Housekeeping',
					filePath: '/hk.pug'
				},
                diag: {
                    type: CdrPlugin.ContentType.PANEL,
                    shortDescription: 'HMC5883 Diag',
                    longDescription: 'HMC5883 Diagnostics',
                    filePath: '/diag.pug'
                }
      }
    };

    return result;
  }
};

//var plugin = new CfeCdrPlugin();

//let CfeCdrPlugin = new class extends CdrPlugin {
