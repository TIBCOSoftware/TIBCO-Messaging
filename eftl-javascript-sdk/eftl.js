/*
 * Copyright (c) 2018: 2018-05-21 11:55:18 -0500 (Mon, 21 May 2018) $ TIBCO Software Inc.
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 *
 * $Id: eftl.js 101362 2018-05-21 16:55:18Z bpeterse $
 */
// Node.js requires a WebSocket implementation (npm install ws)
var WebSocket = WebSocket || require('ws');
(function() {
  var root = this;

  if (_isUndefined(console)) {
    console = {};
    console.log = function(arg) {};
    console.info = function(arg) {};
    console.error = function(arg) {};
  }

  // store connections here to prevent gc
  var openConnections = [];

  var trustCertificates;

  // Minimal Underscore.js inspired utility functions required by eFTL.
  // to provide cross-browser compatibility.
  //
  // Original attribution for isArray(), isDate(), isFunction(), isNumber(),
  // isObject(), isString(), isUndefined(), isNull(), each(), map() and
  // extend():
  //     Underscore.js 1.3.1
  //     (c) 2009-2012 Jeremy Ashkenas, DocumentCloud Inc.
  //     Underscore is freely distributable under the MIT license.
  //     Portions of Underscore are inspired or borrowed from Prototype,
  //     Oliver Steele's Functional, and John Resig's Micro-Templating.
  //     For all details and documentation:
  //     http://documentcloud.github.com/underscore
  var arrayForEach = Array.prototype.forEach;
  var arrayMap = Array.prototype.map;

  var _keys = Object.keys || function(o) {
    if (o !== Object(o)) throw new TypeError('Invalid object');

    var retVal = [];
    for (var k in o) {
      if (Object.prototype.hasOwnProperty.call(o, k)) {
        retVal[retVal.length] = k;
      }
    }
    return retVal;
  };

  function _isArray(a) {
    if (Array.isArray) return Array.isArray(a);
    return Object.prototype.toString.call(a) == '[object Array]';
  }

  function _isDate(d) {
    return Object.prototype.toString.call(d) == '[object Date]';
  }

  function _isFunction(f) {
    return Object.prototype.toString.call(f) == '[object Function]';
  }

  function _isNumber(n) {
    return Object.prototype.toString.call(n) == '[object Number]';
  }

  function _isBoolean(b) {
    return b === true || b === false || Object.prototype.toString.call(b) == '[object Boolean]';
  }

  function _isObject(o) {
    return o === Object(o);
  }

  function _isString(s) {
    return Object.prototype.toString.call(s) == '[object String]';
  }

  function _isUndefined(o) {
    return o === void 0;
  }

  function _isNull(o) {
    return o === null;
  }

  function _isInteger(n) {
    return n % 1 === 0;
  }

  function _each(obj, iterator, context) {
    if (obj == null) return;

    if (arrayForEach && obj.forEach === arrayForEach) {
      obj.forEach(iterator, context);
    } else if (obj.length === +obj.length) {
      for (var i = 0, l = obj.length; i < l; i++) {
        if (i in obj && iterator.call(context, obj[i], i, obj) === {}) return;
      }
    } else {
      for (var key in obj) {
        if (Object.prototype.hasOwnProperty.call(obj, key)) {
          if (iterator.call(context, obj[key], key, obj) === {}) return;
        }
      }
    }
  }

  function _map(obj, iterator, context) {
    if (obj == null) return [];

    if (arrayMap && obj.map === arrayMap) {
      return obj.map(iterator, context);
    }

    var retVal = [];
    _each(obj, function(value, index, list) {
      retVal[retVal.length] = iterator.call(context, value, index, list);
    });

    if (obj.length === +obj.length) {
      retVal.length = obj.length;
    }

    return retVal;
  }

  function _extend(o) {
    _each(Array.prototype.slice.call(arguments, 1), function(source) {
      for (var key in source) {
        if (source.hasOwnProperty(key)) {
          o[key] = source[key];
        }
      }
    });

    return o;
  }

  function _safeExtend(o) {
    _each(Array.prototype.slice.call(arguments, 1), function(source) {
      var v;
      for (var key in source) {
        if (source.hasOwnProperty(key)) {
          v = source[key];
          if (_isNull(v) || _isUndefined(v) || _isFunction(v)) {
            continue;
          }
          if (_isBoolean(v)) {
            v = v.toString();
          }
          o[key] = v;
        }
      }
    });

    return o;
  }

  function assertEFTLMessageInstance(message, variableName) {
    var varName = variableName || "message";
    if (_isUndefined(message)) {
      throw new Error(varName + " cannot be undefined");
    }
    if (_isNull(message)) {
      throw new Error(varName + " cannot be null");
    }
    if (message instanceof eFTLMessage == false) {
      throw new Error(varName + " must be an instance of eFTLMessage");
    }
  }

  if (typeof btoa === 'undefined') {
    btoa = function(str) {
      return new Buffer(str).toString('base64');
    };
  }

  if (typeof atob === 'undefined') {
    atob = function(str) {
      return new Buffer(str, 'base64').toString();
    };
  }

  function parseURL(str) {
    if (typeof module === 'object' && module.exports) {
      // Node.js
      var url = require('url');
      return url.parse(str);
    } else if ("function" == typeof URL) {
        // modern browser and web worker thread
        return new URL(str);
    } else {
      // fallback for older browser
      var url = document.createElement('a');
      url.href = str;
      url.query = (url.search && url.search.charAt(0) === '?' ? url.search.substring(1) : url.search);
      if (str.indexOf('://') !== -1 && str.indexOf('@') !== -1) {
        url.auth = str.substring(str.indexOf('://') + 3, str.indexOf('@'));
      }
      return url;
    }
  }

  function getQueryVariable(url, name) {
    var query = url.query;
    if (query) {
      var vars = query.split('&');
      for (var i = 0; i < vars.length; i++) {
        var pair = vars[i].split('=');
        if (pair[0] == name) {
          return pair[1];
        }
      }
    }
  }

  /**
   * The eFTL library exports a single object called <tt>eFTL</tt> to the JavaScript root context.
   * Programs use the <tt>eFTL</tt> object to connect to an eFTL server.
   *
   * @see eFTL#connect
   * @see eFTL#getVersion
   * @namespace {eFTL} eFTL
   */
  var eFTL = function() {};

  /**
   * @callback onConnect
   * @desc A new connection to the eFTL server is ready to use.
   * @param {eFTLConnection} connection The connection that is ready to use.
   * @see eFTL#connect
   */

  /**
   * @callback onError
   * @desc An error prevented an operation.
   * @param {(eFTLConnection|eFTLMessage|object)} obj The source of the error. For connection errors, this argument is
   * a connection object. For publish errors, this argument is the message that was not published. For subscription errors, this argument is an object that represents a subscription identifier.
   * @param {object} errorCode A code categorizes the error. Your program can use this value in its response logic.
   * @param {string} reason This string provides more detail. Your program can use this value in error reporting and logging.
   * @see eFTL#connect
   * @see eFTLConnection#publish
   * @see eFTLConnection#subscribe
   */

  /**
   * @callback onDisconnect
   * @desc A connection to the eFTL server has closed.
   * @param {eFTLConnection} connection The connection that closed.
   * @param {object} errorCode A code categorizes the error. Your program can use this value in its response logic.
   * @param {string} reason This string provides more detail. Your program can use this value in error reporting and logging.
   * @see eFTL#connect
   */

  /**
   * @callback onReconnect
   * @desc <p>A connection to the eFTL server has re-opened and is ready to use.</p>
   * <p>The eFTL library invokes this method only after your program calls {@link eFTLConnection#reconnect} (and not {@link eFTLConnection#connect}).
   * @param {eFTLConnection} connection The connection that reconnected.
   *
   * @see eFTL#connect
   * @see eFTLConnection#reconnect
   */


  /**
   * <p>Connect to an eFTL server.</p>
   *
   * <p>This call returns immediately; connecting continues asynchronously. When the connection is ready to use,
   * the eFTL library calls your {@link onConnect} callback, passing an <tt>eFTLConnection</tt> object that
   * you can use to publish and subscribe.</p>
   *
   * <p>A program that uses more than one server channel must connect separately to each channel.</p>
   *
   * @param {string} url The call connects to the eFTL server at this URL. The URL can be in either of these forms:
   * <ul>
   *  <li><tt>ws://host:port/channel</tt></li>
   *  <li><tt>wss://host:port/channel</tt></li>
   * </ul>
   * Optionally, the URL can contain the username, password, and/or client identifier:
   * <ul>
   *  <li><tt>ws://username:password@host:port/channel?clientId=&lt;identifier&gt;</tt></li>
   *  <li><tt>wss://username:password@host:port/channel?clientId=&lt;identifier&gt;</tt></li>
   * </ul>
   * @param {object} options A JavaScript object holding properties and callbacks.
   * @memberOf eFTL
   *
   * @param {string} [options.username] Connect using this username if not specified with the URL.
   * @param {string} [options.password] Connect using this password if not specified with the URL.
   * @param {string} [options.clientId] Connect using this client identifier if not specified with the URL.
   * @param {number} [options.autoReconnectAttempts] Specify the maximum number of autoreconnect attempts if the connection is lost.
   * @param {number} [options.autoReconnectMaxDelay] Specify the maximum delay (in milliseconds) between autoreconnect attempts.
   * @param {boolean} [options.trustAll] Specify <code>true</code> to skip eFTL server certificate authentication. 
   * This option should only be used during development and testing. See {@link eFTL#setTrustCertificates}.
   * @param {onConnect} [options.onConnect] A new connection to the eFTL server is ready to use.
   * @param {onError} [options.onError] An error prevented an operation.
   * @param {onDisconnect} [options.onDisconnect] A connection to the eFTL server has closed.
   * @param {onReconnect} [options.onReconnect] A connection to the eFTL server has re-opened and is ready to use.
   *
   * @see eFTLConnection
   *
   */
  eFTL.prototype.connect = function(url, options) {
    if (_isUndefined(url) || _isNull(url)) {
      throw new TypeError("The URL is null or undefined.");
    }
    url = url.trim();
    if (!(url.startsWith('ws:') || url.startsWith('wss:'))) {
      throw new SyntaxError("The URL's scheme must be either 'ws' or 'wss'.");
    }
    var connection = new eFTLConnection();
    connection.open(url, options);
  };

  /**
   * Set the certificates to trust when making a secure connection.
   *
   * <p>Self-signed server certificates are not supported.
   *
   * <p>If you do not set a trust certificate the application trusts
   * any server certificate.
   *
   * <p>This method is not supported by browsers. Browsers only trust
   * certificate authorities that have been installed by the browser.
   *
   * @param {string|Buffer|string[]|Buffer[]} certs Trust these certificates.
   * @memberOf eFTL
   */
  eFTL.prototype.setTrustCertificates = function(certs) {
    trustCertificates = certs;
  }

  /**
   * Get the version of the eFTL Java client library.
   * @returns {string} The version of the eFTL Java client library.
   * @memberOf eFTL
   */
  eFTL.prototype.getVersion = function() {
    return "TIBCO eFTL Version " + EFTL_VERSION;
  };

  var EFTL_VERSION = "@EFTL_VERSION_MAJOR@.@EFTL_VERSION_MINOR@.@EFTL_VERSION_UPDATE@ @EFTL_VERSION_RELEASE_TYPE@ V@EFTL_VERSION_BUILD@";
  var EFTL_WS_PROTOCOL = "v1.eftl.tibco.com";

  var OP_HEARTBEAT = 0;
  var OP_LOGIN = 1;
  var OP_WELCOME = 2;
  var OP_SUBSCRIBE = 3;
  var OP_SUBSCRIBED = 4;
  var OP_UNSUBSCRIBE = 5;
  var OP_UNSUBSCRIBED = 6;
  var OP_EVENT = 7;
  var OP_MESSAGE = 8;
  var OP_ACK = 9;
  var OP_ERROR = 10;
  var OP_DISCONNECT = 11;
  var OP_MAP_SET = 20;
  var OP_MAP_GET = 22;
  var OP_MAP_REMOVE = 24;
  var OP_MAP_RESPONSE = 26;

  var USER_PROP = "user";
  var USERNAME_PROP = "username";
  var PASSWORD_PROP = "password";
  var CLIENT_ID_PROP = "clientId";
  var AUTO_RECONNECT_ATTEMPTS_PROP = "autoReconnectAttempts";
  var AUTO_RECONNECT_MAX_DELAY_PROP = "autoReconnectMaxDelay";

  var OP_FIELD = "op";
  var USER_FIELD = "user";
  var PASSWORD_FIELD = "password";
  var CLIENT_ID_FIELD = "client_id";
  var CLIENT_TYPE_FIELD = "client_type";
  var CLIENT_VERSION_FIELD = "client_version";
  var ID_TOKEN_FIELD = "id_token";
  var TIMEOUT_FIELD = "timeout";
  var HEARTBEAT_FIELD = "heartbeat";
  var MAX_SIZE_FIELD = "max_size";
  var MATCHER_FIELD = "matcher";
  var DURABLE_FIELD = "durable";
  var ACK_FIELD = "ack";
  var ERR_CODE_FIELD = "err";
  var REASON_FIELD = "reason";
  var ID_FIELD = "id";
  var MSG_FIELD = "msg";
  var TO_FIELD = "to";
  var BODY_FIELD = "body";
  var SEQ_NUM_FIELD = "seq";
  var DOUBLE_FIELD = "_d_";
  var MILLISECOND_FIELD = "_m_";
  var OPAQUE_FIELD = "_o_";
  var LOGIN_OPTIONS_FIELD = "login_options";
  var RESUME_FIELD = "_resume";
  var QOS_FIELD = "_qos";
  var CONNECT_TIMEOUT = "timeout";
  var MAP_FIELD = "map";
  var KEY_FIELD = "key";
  var VALUE_FIELD = "value";

  var KEY_TYPE = ".k.";
  var SUBSCRIPTION_TYPE = ".s.";

  function doubleToJson(value, key, list) {
    var retVal = {};
    if (isFinite(value))
      retVal[DOUBLE_FIELD] = value;
    else
      retVal[DOUBLE_FIELD] = value.toString();
    return retVal;
  }

  function jsonToDouble(raw) {
    if (_isNumber(raw[DOUBLE_FIELD]))
      return raw[DOUBLE_FIELD];
    else
      return parseFloat(raw[DOUBLE_FIELD]);
  }

  function dateToJson(value, key, list) {
    var millis = value.getTime();
    var retVal = {};
    retVal[MILLISECOND_FIELD] = millis;
    return retVal;
  }

  function jsonToDate(raw) {
    if (raw[MILLISECOND_FIELD] !== undefined) {
      var millis = raw[MILLISECOND_FIELD];
      return new Date(millis);
    }
    return undefined;
  }

  function opaqueToJson(value, key, list) {
    var retVal = {};
    retVal[OPAQUE_FIELD] = btoa(value);
    return retVal;
  }

  function jsonToOpaque(raw) {
    if (raw[OPAQUE_FIELD] !== undefined) {
      return atob(raw[OPAQUE_FIELD]);
    }
    return undefined;
  }

  /**
   * Message objects contain typed fields that map names to values.
   * @class eFTLMessage
   */
  function eFTLMessage() {
    if (arguments.length === 1) {
      _safeExtend(this, arguments[0]);
    }
  }

  /**
   * Set a field in a message.
   *
   *  <p>Note: This method can set values of the following types:
   *  <ul>
   *  <li>String</li>
   *  <li>Number</li>
   *  <li>Date</li>
   *  <li>eFTLMessage</li>
   *  <li>Array of String (null/undefined is not a legal value within this array.)</li>
   *  <li>Array of Number</li>
   *  <li>Array of Date</li>
   *  <li>Array of eFTLMessage</li>
   *  </ul>
   * <p> The method marks the field with the corresponding field type.
   * <p> To set a value and mark the field with a different field
   *  type, use the following methods:
   * <ul>
   * <li> {@link eFTLMessage#setAsLong}
   * <li> {@link eFTLMessage#setAsDouble}
   * <li> {@link eFTLMessage#setAsOpaque}
   * </ul>
   *
   *
   * @memberOf eFTLMessage
   * @param {string} fieldName Set this field
   * @param {object} value Set this value. To remove the field, supply <tt>null</tt>.
   *
   *  @throws TypeError if the value provided doesn't not match a supported type.
   */
  eFTLMessage.prototype.set = function(fieldName, value) {
    var val = undefined;

    if (value instanceof eFTLMessage) {
      val = value;
    } else if (_isArray(value)) {
      if (value.length > 0) {
        if (value[0] instanceof eFTLMessage) {
          val = _map(value, function(o) {
            return o;
          });
        } else if (_isDate(value[0])) {
          val = _map(value, dateToJson);
        } else if (_isString(value[0])) {
          val = value;
        } else if (_isNumber(value[0])) {
          if (_isInteger(value[0])) {
            val = value;
          } else {
            val = _map(value, doubleToJson);
          }
        } else {
          throw new TypeError("Unsupported object type: " + typeof value);
        }
      } else {
        val = value;
      }
    } else if (_isDate(value)) {
      val = dateToJson(value);
    } else if (_isString(value)) {
      val = value;
    } else if (_isNumber(value)) {
      if (_isInteger(value)) {
        val = value;
      } else {
        val = doubleToJson(value);
      }
    } else if (_isNull(value) === false && _isUndefined(value) === false) {
      throw new TypeError("Unsupported object type: " + typeof value);
    }

    if (_isUndefined(val)) {
      delete this[fieldName];
    } else {
      this[fieldName] = val;
    }
  };

  /**
   * Set a field in a message, marking the field as opaque.
   *
   * @memberOf eFTLMessage
   * @param {string} fieldName Set this field
   * @param {string} value Set this value. To remove, supply <tt>null</tt>.
   */
  eFTLMessage.prototype.setAsOpaque = function(fieldName, value) {
    if (_isNull(value) || _isUndefined(value)) {
      delete this[fieldName];
    } else {
      this[fieldName] = opaqueToJson(value);
    }
  }

  /**
   * Set a field in a message, marking the field as <tt>long</tt>.
   *
   * @memberOf eFTLMessage
   * @param {string} fieldName Set this field
   * @param {number} value Set this value. To remove, supply <tt>null</tt>.
   *
   *  @throws TypeError if the value provided doesn't not match a Number or an array of Number.
   */
  eFTLMessage.prototype.setAsLong = function(fieldName, value) {
    var val = undefined;

    if (_isArray(value)) {
      if (value.length > 0) {
        if (_isNumber(value[0])) {
          val = _map(value, Math.floor);
        } else {
          throw new TypeError("Unsupported object type: " + typeof value);
        }
      } else {
        val = value;
      }
    } else if (_isNumber(value)) {
      val = Math.floor(value);
    } else if (_isNull(value) === false && _isUndefined(value) === false) {
      throw new TypeError("Unsupported object type: " + typeof value);
    }

    if (_isUndefined(val)) {
      delete this[fieldName];
    } else {
      this[fieldName] = val;
    }
  };

  /**
   * Set a field in the message, marking the field as <tt>double</tt>.
   * @memberOf eFTLMessage
   * @param {string} fieldName Set this field
   * @param {number} value Set this value. To remove, supply <tt>null</tt>.
   *
   *  @throws TypeError if the value provided is not a Number or an array of Number.
   */
  eFTLMessage.prototype.setAsDouble = function(fieldName, value) {
    var val = undefined;

    if (_isArray(value)) {
      if (value.length > 0) {
        if (_isNumber(value[0])) {
          val = _map(value, doubleToJson);
        } else {
          throw new TypeError("Unsupported object type: " + typeof value);
        }
      } else {
        val = value;
      }
    } else if (_isNumber(value)) {
      val = doubleToJson(value);
    } else if (_isNull(value) === false && _isUndefined(value) === false) {
      throw new TypeError("Unsupported object type: " + typeof value);
    }

    if (_isUndefined(val)) {
      delete this[fieldName];
    } else {
      this[fieldName] = val;
    }
  };


  /**
   * Get the value of a field from a message.
   * @memberOf eFTLMessage
   * @param {string} fieldName Get this field.
   * @returns {object} The value of the field, if the field is present; <tt>undefined</tt> otherwise.
   */
  eFTLMessage.prototype.get = function(fieldName) {

    var raw = this[fieldName];
    var retVal = undefined;

    if (_isArray(raw)) {
      if (raw.length > 0) {
        if (_isObject(raw[0])) {
          if (raw[0][DOUBLE_FIELD]) {
            retVal = _map(raw, jsonToDouble);
          } else if (raw[0][MILLISECOND_FIELD]) {
            retVal = _map(raw, jsonToDate);
          } else {
            retVal = _map(raw, function(o) {
              return new eFTLMessage(o);
            });
          }
        } else {
          retVal = raw;
        }
      } else {
        retVal = raw;
      }
    } else if (_isObject(raw)) {
      if (raw[DOUBLE_FIELD] !== undefined) {
        retVal = jsonToDouble(raw);
      } else if (raw[MILLISECOND_FIELD] !== undefined) {
        retVal = jsonToDate(raw);
      } else if (raw[OPAQUE_FIELD] !== undefined) {
        retVal = jsonToOpaque(raw);
      } else {
        retVal = new eFTLMessage(raw);
      }
    } else {
      retVal = raw;
    }

    return retVal;
  };

  // Returns message field names on this message

  /**
   * Get the names of all fields present in the message.
   * @memberOf eFTLMessage
   * @returns {String[]} An array of of field names.
   */
  eFTLMessage.prototype.getFieldNames = function() {

    var buf = [];
    _each(_keys(this), function(kn) {
      buf.push(kn);
    });
    return buf;
  };

  /**
   * Returns a string representing the message.
   * @memberOf eFTLMessage
   * @returns {string} A string representing the message.
   */
  eFTLMessage.prototype.toString = function() {
    var msg = this;
    var str = "{";
    _each(_keys(this), function(kn) {
      var val = msg.get(kn);
      if (str.length > 1)
        str += ", ";
      str += kn;
      str += "=";
      if (_isArray(val)) {
        str += "[";
        str += val;
        str += "]";
      } else {
        str += val;
      }
    });
    str += "}";
    return str;
  };

  /**
   * @callback eFTLKVMap~onSuccess
   * @desc A key-value map operation has completed successfully.
   *
   * @param {string} key The key being operated upon.
   * @param {eFTLMessage} message The value of the key.
   * @see eFTLKVMap#set
   * @see eFTLKVMap#get
   * @see eFTLKVMap#remove
   */

  /**
   * @callback eFTLKVMap~onError
   * @desc An error prevented a key-value map operation.
   * @param {string} key The key being operated upon.
   * @param {eFTLMessage} message The value of the key.
   * @param {object} errorCode A code categorizes the error. Your program can use this value in its response logic.
   * @param {string} reason This string provides more detail. Your program can use this value in error reporting and logging.
   * @see eFTLKVMap#set
   * @see eFTLKVMap#get
   * @see eFTLKVMap#remove
   */

  /**
   * <p>A key-value map object allows setting, getting, and removing key-value pairs
   * in an FTL map.</p>
   *
   * @class eFTLKVMap
   *
   * @see eFTLConnection#createKVMap
   */
  function eFTLKVMap(connection, name) {
    this.connection = connection;
    this.name = name;
  }

  /**
   * <p>Set a key-value pair in the map, overwriting any existing value.</p>
   *
   * @memberOf eFTLKVMap
   * @param {key} key Set the value for this key.
   * @param {eFTLMessage} message Set this value for the key.
   * @param {object} options A JavaScript object holding key-value map callbacks.
   * @param {eFTLKVMap~onSuccess} [options.onSuccess] Process success.
   * @param {eFTLKVMap~onError} [options.onError] Process errors.
   */
  eFTLKVMap.prototype.set = function(key, message, options) {
    assertEFTLMessageInstance(message, "Message");
    
    var connection = this.connection;

    var envelope = {};
    envelope[OP_FIELD] = OP_MAP_SET;
    envelope[MAP_FIELD] = this.name;
    envelope[KEY_FIELD] = key;

    var value = new eFTLMessage(message);
    envelope[VALUE_FIELD] = value;

    var sequence = connection._nextSequence();
    if (connection.qos) {
      envelope[SEQ_NUM_FIELD] = sequence;
    }

    var request = {};
    request.message = message;
    request.envelope = envelope;
    request.sequence = sequence;
    request.options = {
      onComplete: function(message) {
        if (options && options.onSuccess) {
          options.onSuccess(key, message);
        }
      },
      onError: function(message, code, reason) {
        if (options && options.onError) {
          options.onError(key, message, code, reason);
        }
      }
    };

    var data = JSON.stringify(envelope);

    if (connection.maxMessageSize > 0 && data.length > connection.maxMessageSize)
      throw new Error("Message exceeds maximum message size of " + connection.maxMessageSize);

    connection.requests[sequence] = request;

    if (connection.state === OPEN) {
      connection._send(envelope, sequence, false, data);
    }
  };

  /**
   * <p>Get a key-value pair from the map.</p>
   *
   * @memberOf eFTLKVMap
   * @param {key} key Get the value for this key.
   * @param {object} options A JavaScript object holding key-value map callbacks.
   * @param {eFTLKVMap~onSuccess} [options.onSuccess] Process success.
   * @param {eFTLKVMap~onError} [options.onError] Process errors.
   */
  eFTLKVMap.prototype.get = function(key, options) {
    var connection = this.connection;

    var envelope = {};
    envelope[OP_FIELD] = OP_MAP_GET;
    envelope[MAP_FIELD] = this.name;
    envelope[KEY_FIELD] = key;

    var sequence = connection._nextSequence();
    envelope[SEQ_NUM_FIELD] = sequence;

    var request = {};
    request.envelope = envelope;
    request.sequence = sequence;
    request.options = {
      onComplete: function(message) {
        if (options && options.onSuccess) {
          options.onSuccess(key, message);
        }
      },
      onError: function(message, code, reason) {
        if (options && options.onError) {
          options.onError(key, message, code, reason);
        }
      }
    };

    var data = JSON.stringify(envelope);

    connection.requests[sequence] = request;

    if (connection.state === OPEN) {
      connection._send(envelope, sequence, false, data);
    }
  };

  /**
   * <p>Remove a key-value pair from the map.</p>
   *
   * @memberOf eFTLKVMap
   * @param {key} key Remove the value for this key.
   * @param {object} options A JavaScript object holding key-value map callbacks.
   * @param {eFTLKVMap~onSuccess} [options.onSuccess] Process success.
   * @param {eFTLKVMap~onError} [options.onError] Process errors.
   */
  eFTLKVMap.prototype.remove = function(key, options) {
    var connection = this.connection;

    var envelope = {};
    envelope[OP_FIELD] = OP_MAP_REMOVE;
    envelope[MAP_FIELD] = this.name;
    envelope[KEY_FIELD] = key;

    var sequence = connection._nextSequence();
    if (connection.qos) {
      envelope[SEQ_NUM_FIELD] = sequence;
    }

    var publish = {};
    publish.envelope = envelope;
    publish.sequence = sequence;
    publish.options = {
      onComplete: function(message) {
        if (options && options.onSuccess) {
          options.onSuccess(key, message);
        }
      },
      onError: function(message, code, reason) {
        if (options && options.onError) {
          options.onError(key, message, code, reason);
        }
      }
    };

    var data = JSON.stringify(envelope);

    connection.requests[sequence] = publish;

    if (connection.state === OPEN) {
      connection._send(envelope, sequence, false, data);
    }
  };

  var OPENING = 0;
  var OPEN = 1;
  var CLOSING = 2;
  var CLOSED = 3;

  /**
   * <p>A connection object represents a program's connection to an eFTL server.</p>
   * <p>The eFTL JavaScript library creates new instances of eFTLConnection internally.</p>
   * <p>Programs use connection objects to send messages, and subscribe to messages.</p>
   * <p>Programs receive connection objects through {@link onConnect}, {@link onError}, {@link onDisconnect},
   * and {@link onReconnect} callbacks.</p>
   *
   * @class eFTLConnection
   *
   * @see eFTL#connect
   */
  function eFTLConnection() {
    this.connectOptions = null;
    this.accessPointURL = null;
    this.webSocket = null;
    this.state = CLOSED;
    this.clientId = null;
    this.reconnectToken = null;
    this.timeout = 600000;
    this.heartbeat = 240000;
    this.maxMessageSize = 0;
    this.lastMessage = 0;
    this.timeoutCheck = null;
    this.subscriptions = {};
    this.sequenceCounter = 0;
    this.subscriptionCounter = 0;
    this.reconnectCounter = 0;
    this.autoReconnectAttempts = 5;
    this.autoReconnectMaxDelay = 30000;
    this.reconnectTimer = null;
    this.connectTimer = null;
    this.isReconnecting = false;
    this.isOpen = false;
    this.qos = true;
    this.requests = {};
    this.lastSequenceNumber = 0;
  }

  // Function to call user code callbacks, and log a message if an exception
  eFTLConnection.prototype._caller = function(func, context, args) {
    try {
      if (func) {
        func.apply(context, args);
      }
    } catch (ex) {
      console.error("Exception while calling client callback", ex, ex.stack);
    }
  };

  /**
   * Determine whether this connection to the eFTL server is open or closed.
   * @memberOf eFTLConnection
   * @returns {boolean} <tt>true</tt> if this connection is open; <tt>false</tt> otherwise.
   */
  eFTLConnection.prototype.isConnected = function() {
    return this.state === OPEN;
  };

  /**
   * Create an {@link eFTLMessage}.
   * @memberOf eFTLConnection
   * @returns {eFTLMessage} A new message.
   */
  eFTLConnection.prototype.createMessage = function() {
    return new eFTLMessage();
  }

  /**
   * Create an {@link eFTLKVMap}.
   * @memberOf eFTLConnection
   * @param {string} name The key-value map name.
   * @returns {eFTLKVMap} A new key-value map.
   */
  eFTLConnection.prototype.createKVMap = function(name) {
    return new eFTLKVMap(this, name);
  }

  eFTLConnection.prototype._send = function(message, seqNum, forceSend) {
    // the 4th argument provided by a call is already stringified - used by heartbeats
    var data = arguments.length === 4 ? arguments[3] : JSON.stringify(message);

    this._debug('>>', message);

    if (this.webSocket) {
      if (this.state < CLOSING || forceSend) {
        try {
          this.webSocket.send(data);

          if (!this.qos && seqNum > 0) {
            this._pendingComplete(seqNum);
          }
        } catch (err) {
          // an error will be thrown when not connected
        }
      }
    }
  };

  eFTLConnection.prototype._subscribe = function(subId, options) {
    var subscription = {};
    subscription.options = options;
    subscription.id = subId;
    subscription.pending = true;

    var subMessage = {};
    subMessage[OP_FIELD] = OP_SUBSCRIBE;
    subMessage[ID_FIELD] = subId;
    subMessage[MATCHER_FIELD] = options.matcher;
    subMessage[DURABLE_FIELD] = options.durable;

    //include options like loopback
    var opts = _safeExtend({}, options);
    delete opts["matcher"];
    delete opts["durable"];
    _extend(subMessage, opts);

    this.subscriptions[subId] = subscription;
    this._send(subMessage, 0, false);

    return subId;
  };

  eFTLConnection.prototype._ack = function(sequenceNumber) {
    if (!this.qos || !sequenceNumber) {
      return;
    }

    var envelope = {};
    envelope[OP_FIELD] = OP_ACK;
    envelope[SEQ_NUM_FIELD] = sequenceNumber;
    this._send(envelope, 0, false);
  };


  eFTLConnection.prototype._sendMessage = function(message, options) {
    var envelope = {};

    envelope[OP_FIELD] = OP_MESSAGE;

    var body = new eFTLMessage(message);
    envelope[BODY_FIELD] = body;

    var sequence = this._nextSequence();
    if (this.qos) {
      envelope[SEQ_NUM_FIELD] = sequence;
    }

    var publish = {};
    publish.options = options;
    publish.message = message;
    publish.envelope = envelope;
    publish.sequence = sequence;

    var data = JSON.stringify(envelope);

    if (this.maxMessageSize > 0 && data.length > this.maxMessageSize)
      throw new Error("Message exceeds maximum message size of " + this.maxMessageSize);

    this.requests[sequence] = publish;

    if (this.state === OPEN) {
      this._send(envelope, sequence, false, data);
    }
  };

  eFTLConnection.prototype._disconnect = function() {
    if (this.reconnectTimer) {
      clearTimeout(this.reconnectTimer);
      this.reconnectTimer = null;
      this._clearPending(11, "Disconnected");
      this._caller(this.connectOptions.onDisconnect, this.connectOptions, [this, 1000, "Disconnect"]);
    }

    this._close("Connection closed by user.", true);
  }

  eFTLConnection.prototype._close = function(reason, notifyServer) {
    if (this.state !== OPENING && this.state !== OPEN) return;

    this.state = CLOSING;

    if (this.timeoutCheck) {
      clearInterval(this.timeoutCheck);
      this.timeoutCheck = null;
    }

    if (notifyServer) {
      reason = reason || "User Action";

      var dcMessage = {};

      dcMessage[OP_FIELD] = OP_DISCONNECT;
      dcMessage[REASON_FIELD] = reason;

      this._send(dcMessage, 0, true);
    }
    this.webSocket.close();
  };

  eFTLConnection.prototype._handleSubscribed = function(message) {
    var subId = message[ID_FIELD];

    var sub = this.subscriptions[subId];
    if (sub != null && sub.pending) {
      // don't notify when we restore subscriptions.
      sub.pending = false;
      var options = sub.options;
      this._caller(options.onSubscribe, options, [subId]);
    }
  };


  eFTLConnection.prototype._handleUnsubscribed = function(message) {
    // server failed at creating the subscription
    var subId = message[ID_FIELD];
    var errCode = message[ERR_CODE_FIELD];
    var reason = message[REASON_FIELD];

    var sub = this.subscriptions[subId];
    if (sub != null) {
      delete this.subscriptions[subId];
      var options = sub.options;
      this._caller(options.onError, options, [subId, errCode, reason]);
    }
  };

  eFTLConnection.prototype._handleEvent = function(message) {
    var to = message[TO_FIELD];
    var seqNum = message[SEQ_NUM_FIELD];
    var data = message[BODY_FIELD];
    var sub = this.subscriptions[to];

    if (!this.qos || !seqNum || seqNum > this.lastSequenceNumber) {
      if (sub && sub.options != null) {
        var wsMessage = new eFTLMessage(data);
        this._debug('<<', data);
        this._caller(sub.options.onMessage, sub.options, [wsMessage]);
      }

      if (this.qos && seqNum) {
        this.lastSequenceNumber = seqNum;
      }
    }
    // ack last sequence number, server will ack all messages less or equal
    this._ack(seqNum);
  };

  eFTLConnection.prototype._handleEvents = function(messages) {
    for (var i = 0, max = messages.length; i < max; i++) {
      var message = messages[i];
      this._handleEvent(message);
    }
  };

  eFTLConnection.prototype._initWS = function(webSocket) {
    var connection = this;

    webSocket.onopen = function(arg) {
      var url = parseURL(connection.accessPointURL);

      var auth = (url.auth ? url.auth.split(':') : []);
      var username = auth[0];
      var password = auth[1];
      var clientId = getQueryVariable(url, "clientId");

      var loginMessage = {};

      loginMessage[OP_FIELD] = OP_LOGIN;
      loginMessage[CLIENT_TYPE_FIELD] = "js";
      loginMessage[CLIENT_VERSION_FIELD] = EFTL_VERSION;

      if (username) {
        loginMessage[USER_FIELD] = username;
      } else if (connection.connectOptions.hasOwnProperty(USERNAME_PROP)) {
        loginMessage[USER_FIELD] = connection.connectOptions[USERNAME_PROP];
      } else {
        loginMessage[USER_FIELD] = connection.connectOptions[USER_PROP];
      }

      if (password) {
        loginMessage[PASSWORD_FIELD] = password;
      } else {
        loginMessage[PASSWORD_FIELD] = connection.connectOptions[PASSWORD_PROP];
      }

      if (connection.clientId && connection.reconnectToken) {
        loginMessage[CLIENT_ID_FIELD] = connection.clientId;
        loginMessage[ID_TOKEN_FIELD] = connection.reconnectToken;
      } else if (clientId) {
        loginMessage[CLIENT_ID_FIELD] = clientId;
      } else {
        loginMessage[CLIENT_ID_FIELD] = connection.connectOptions[CLIENT_ID_PROP];
      }

      if (!_isBoolean(connection.connectOptions[QOS_FIELD])) {
        connection.connectOptions[QOS_FIELD] = true;
      }
      connection.qos = connection.connectOptions[QOS_FIELD];

      if (_isNumber(connection.connectOptions[AUTO_RECONNECT_ATTEMPTS_PROP])) {
        connection.autoReconnectAttempts = connection.connectOptions[AUTO_RECONNECT_ATTEMPTS_PROP];
      }

      if (_isNumber(connection.connectOptions[AUTO_RECONNECT_MAX_DELAY_PROP])) {
        connection.autoReconnectMaxDelay = connection.connectOptions[AUTO_RECONNECT_MAX_DELAY_PROP];
      }

      var options = _safeExtend({}, connection.connectOptions);
      if (connection.isReconnecting) {
        options[RESUME_FIELD] = "true";
      }

      var removeOptions = [USER_PROP, USERNAME_PROP, PASSWORD_PROP, CLIENT_ID_PROP];
      _each(removeOptions, function(value) {
        delete options[value];
      });

      loginMessage[LOGIN_OPTIONS_FIELD] = options;

      connection._send(loginMessage, 0, false);
    };

    webSocket.onerror = function(error) {};

    webSocket.onclose = function(evt) {
      if (webSocket === null) return;

      if (connection.state === CLOSED) return;

      connection.state = CLOSED;

      if (connection.connectTimer) {
          clearTimeout(connection.connectTimer);
          connection.connectTimer = null;
      }

      if (connection.timeoutCheck) {
        clearInterval(connection.timeoutCheck);
        connection.timeoutCheck = null;
      }

      connection.webSocket = null;
      var index = openConnections.indexOf(this);
      if (index !== -1) {
        openConnections.splice(index, 1);
      }

      // Not every browser is setting the wasClean flag. Therefore,
      // also check for a 1006 code which indicates an abnormal
      // closure (i.e., the server did not send a close frame).
      if (evt.wasClean || evt.code != 1006) {
        connection.isOpen = false;
        connection._clearPending(11, "Closed");
        connection._caller(connection.connectOptions.onDisconnect, connection.connectOptions, [connection, evt.code, (evt.reason ? evt.reason : "Connection failed")]);
      } else if (!connection.isOpen || !connection._scheduleReconnect()) {
        connection.isOpen = false;
        connection._clearPending(11, "Closed");
        connection._caller(connection.connectOptions.onDisconnect, connection.connectOptions, [connection, evt.code, (evt.reason ? evt.reason : "Connection failed")]);
      }
    };

    webSocket.onmessage = function(rawMessage) {
      var msg = JSON.parse(rawMessage.data);
      connection._debug("<<", msg);

      connection.lastMessage = (new Date()).getTime();

      if (_isArray(msg)) {
        try {
          connection._handleEvents(msg);
        } catch (err) {
          console.error('Exception on onmessage callback.', err, err.stack);
        }
      } else {
        try {
          if (!_isUndefined(msg[OP_FIELD])) {
            var opCode = msg[OP_FIELD];

            switch (opCode) {
              case OP_HEARTBEAT:
                // bypass serialization the object, give the rawmessage data
                connection._send.apply(connection, [msg, 0, false, rawMessage.data]);
                break;
              case OP_WELCOME:
                connection._handleWelcome(msg);
                break;
              case OP_SUBSCRIBED:
                connection._handleSubscribed(msg);
                break;
              case OP_UNSUBSCRIBED:
                connection._handleUnsubscribed(msg);
                break;
              case OP_EVENT:
                connection._handleEvent(msg);
                break;
              case OP_ERROR:
                connection._handleError(msg);
                break;
              case OP_ACK:
                connection._handleAck(msg);
                break;
              case OP_MAP_RESPONSE:
                connection._handleMapResponse(msg);
                break;
              default:
                console.log("Received unknown/unexpected op code - " + opCode, rawMessage.data);
                break;
            }
          }
        } catch (err) {
          console.error(err, err.stack);
        }
      }
    };
  };

  eFTLConnection.prototype._handleWelcome = function(message) {

    // a non-null reconnect token indicates a prior connection
    var reconnectId = (this.reconnectToken != null);
    var resume = ((message[RESUME_FIELD] || "") + "").toLowerCase() === "true";
    this.clientId = message[CLIENT_ID_FIELD];
    this.reconnectToken = message[ID_TOKEN_FIELD];
    this.timeout = 1000 * message[TIMEOUT_FIELD];
    this.heartbeat = 1000 * message[HEARTBEAT_FIELD];
    this.maxMessageSize = message[MAX_SIZE_FIELD];


    if (!resume) {
      this._clearPending(11, "Reconnect");
      this.lastSequenceNumber = 0;
    }

    this.qos = ((message[QOS_FIELD] || "") + "").toLowerCase() === "true";

    var connection = this;

    if (this.heartbeat > 0) {
      this.timeoutCheck = setInterval(function() {
        var now = (new Date()).getTime();
        var diff = now - connection.lastMessage;

        if (diff > connection.timeout) {
          connection._debug("lastMessage in timer ", {
            id: connection.clientId,
            now: now,
            lastMessage: connection.lastMessage,
            timeout: connection.timeout,
            diff: diff,
            evaluate: diff > connection.timeout
          });
          connection._close("Connection timeout", false);
        }
      }, this.heartbeat);
    }

    this.state = OPEN;
    this.reconnectCounter = 0;

    if (_keys(this.subscriptions).length > 0 && !resume) {
      _each(this.subscriptions, function(value, key) {
        value.pending = true;
      });
    }

    if (_keys(this.subscriptions).length > 0) {
      _each(this.subscriptions, function(value, key) {
        if (value.pending) {
          connection._subscribe(key, value.options);
        }
      });
    }

    this._sendPending();

    if (reconnectId && !this.isReconnecting) {
      this._caller(this.connectOptions.onReconnect, this.connectOptions, [this]);
    } else if (!this.isReconnecting) {
      this._caller(this.connectOptions.onConnect, this.connectOptions, [this]);
    }

    this.isReconnecting = false;
    this.isOpen = true;
  };

  eFTLConnection.prototype._handleError = function(message) {
    var errCode = message[ERR_CODE_FIELD];
    var reason = message[REASON_FIELD];

    this._caller(this.connectOptions.onError, this.connectOptions, [this, errCode, reason]);

    if (this.state === OPENING) {
      this._close(reason, false);
    }
  };

  eFTLConnection.prototype._handleAck = function(message) {
    var sequence = message[SEQ_NUM_FIELD];
    var errCode = message[ERR_CODE_FIELD];
    var reason = message[REASON_FIELD];
    if (!_isUndefined(sequence)) {
      if (_isNull(errCode) || _isUndefined(errCode)) {
        this._pendingComplete(sequence);
      } else {
        this._pendingError(sequence, errCode, reason);
      }
    }
  };

  eFTLConnection.prototype._handleMapResponse = function(message) {
    var sequence = message[SEQ_NUM_FIELD];
    var value = message[VALUE_FIELD];
    var errCode = message[ERR_CODE_FIELD];
    var reason = message[REASON_FIELD];
    if (!_isUndefined(sequence)) {
      if (_isUndefined(errCode) || _isNull(errCode)) {
        if (_isUndefined(value) || _isNull(value)) {
          this._pendingComplete(sequence);
        } else {
          this._pendingResponse(sequence, new eFTLMessage(value));
        }
      } else {
        this._pendingError(sequence, errCode, reason);
      }
    }
  };

  eFTLConnection.prototype._sendPending = function() {
    var sequences = _keys(this.requests);
    if (sequences.length > 0) {
      var self = this;
      sequences.sort(function(a, b) {
        return a - b;
      });
      _each(sequences, function(obj) {
        var seq = obj;
        var req = self.requests[seq];
        self._send(req.envelope, req.sequence, false);
      });
    }
  };

  eFTLConnection.prototype._pendingComplete = function(sequence) {
    var req = this.requests[sequence];
    if (req != null) {
      delete this.requests[sequence];
      if (req.options != null) {
        this._caller(req.options.onComplete, req.options, [req.message]);
      }
    }
  }

  eFTLConnection.prototype._pendingResponse = function(sequence, response) {
    var req = this.requests[sequence];
    if (req != null) {
      delete this.requests[sequence];
      if (req.options != null) {
        this._caller(req.options.onComplete, req.options, [response]);
      }
    }
  }

  eFTLConnection.prototype._pendingError = function(sequence, errCode, reason) {
    var req = this.requests[sequence];
    if (req != null) {
      delete this.requests[sequence];
      if (req.options != null) {
        this._caller(req.options.onError, req.options, [req.message, errCode, reason]);
      } else {
        this._caller(this.connectOptions.onError, this.connectOptions, [this, errCode, reason]);
      }
    }
  }

  eFTLConnection.prototype._clearPending = function(errCode, reason) {
    var sequences = _keys(this.requests);
    if (sequences.length > 0) {
      var self = this;
      sequences.sort(function(a, b) {
        return a - b;
      });
      _each(sequences, function(obj) {
        var seq = obj;
        var req = self.requests[seq];
        if (req.options != null) {
          self._caller(req.options.onError, req.options, [req.message, errCode, reason]);
        }
        delete self.requests[seq];
      });
    }
  };

  /**
   * Returns the next sequence number for a message
   * @returns {number}
   * @private
   */
  eFTLConnection.prototype._nextSequence = function() {
    return ++this.sequenceCounter;
  };

  /**
   * Returns the next sequence number for a subscription
   * @returns {number}
   * @private
   */
  eFTLConnection.prototype._nextSubscriptionSequence = function() {
    return ++this.subscriptionCounter;
  };

  eFTLConnection.prototype.open = function(url, options) {
    this.connectOptions = options;
    this.accessPointURL = url;
    this.state = OPENING;

    if (url.indexOf('?') > 0) {
      url = url.substring(0, url.indexOf('?'));
    }

    try {
      var options = {};

      if (trustCertificates !== undefined) {
        options.ca = trustCertificates;
      }

      if (_isBoolean(this.connectOptions.trustAll) && this.connectOptions.trustAll) {
        options.rejectUnauthorized = false;
      }

      this.webSocket = new WebSocket(url, [EFTL_WS_PROTOCOL], options);
      this._initWS(this.webSocket);

      openConnections.push(this);

      // timeout a connect request after 15 seconds

      var timeout = 15000;

      if (_isNumber(this.connectOptions[CONNECT_TIMEOUT]))
        timeout = this.connectOptions[CONNECT_TIMEOUT] * 1000;

      var webSocket = this.webSocket

      this.connectTimer = setTimeout(function() {
        if (webSocket.readyState === WebSocket.CONNECTING) {
          webSocket.close();
        }
      }, timeout);
    } catch (err) {
      // log it, and throw it.
      console.error(err, err.stack);
      throw err;
    }
  };

  /**
   * @callback onSubscribe
   * @desc <p>A new subscription is ready to receive messages.</p>
   * <p>The eFTL library may invoke this method after the first message arrives.</p>
   * <p>To close the subscription, call {@link eFTLConnection#unsubscribe} with the subscription identifier.</p>
   *
   * @param {object} subscriptionId The subscription identifier.
   * @see eFTLConnection#subscribe
   * @see eFTLConnection#unsubscribe
   */

  /**
   * @callback onMessage
   *
   * @desc <p>Process an inbound message.</p>
   * <p>The eFTL library presents inbound messages to this callback for processing. You must implement this callback
   * to process messages.</p>
   *
   * @param {eFTLMessage} message The inbound message.
   * @see eFTLConnection#subscribe
   */

  /**
   * <p>Subscribe to messages.</p>
   * <p>Register a subscription for one-to-many messages.</p>
   * <p>This call returns immediately; subscribing continues asynchronously. When the subscription is ready to
   * receive messages, the eFTL library calls your {@link onSubscribe} callback.</p>
   *
   * <p>A matcher can narrow subscription interest in the inbound message stream.</p>
   * <p>It is good practice to subscribe to
   * messages published to a specific destination
   * by using the message field name <code>_dest</code>.
   * <p>
   * @example
   * <caption>To subscribe to messages published to a specific destination,
   * create a subscription matcher for that destination;
   * for example:</caption>
   * var matcher = '{"_dest":"myTopic"}';
   *
   * @memberOf eFTLConnection
   * @param {object} options A JavaScript object holding subscription callbacks.
   *
   * @param {string} [options.matcher] The subscription uses this content matcher to narrow the message stream.
   * @param {string} [options.durable] The subscription is associated with this durable name.
   * @param {string} [options.type] An optional durable type; 'shared' or 'last-value'.
   * @param {string} [options.key] The key field for 'last-value' durable subscriptions.
   * @param {onSubscribe} [options.onSubscribe] The new subscription is ready.
   * @param {onError} [options.onError] Process subscription errors.
   * @param {onMessage} [options.onMessage] Process inbound messages
   * @returns {object} The subscription identifier.
   *
   * @see eFTLConnection#unsubscribe
   */
  eFTLConnection.prototype.subscribe = function(options) {
    var subId = this.clientId + SUBSCRIPTION_TYPE + this._nextSubscriptionSequence();
    this._subscribe(subId, options);
    return subId;
  };

  /**
   * <p>Close a subscription.</p>
   * <p>Programs receive subscription identifiers via their {@link onSubscribe} handlers.</p>
   * @memberOf eFTLConnection
   * @param {object} subscriptionId Close this subscription.
   * @see eFTLConnection#subscribe
   */
  eFTLConnection.prototype.unsubscribe = function(subscriptionId) {
    var unsubMessage = {};
    unsubMessage[OP_FIELD] = OP_UNSUBSCRIBE;
    unsubMessage[ID_FIELD] = subscriptionId;

    this._send(unsubMessage, 0, false);
    delete this.subscriptions[subscriptionId];
  };

  /**
   * <p>Close all subscriptions.</p>
   * @memberOf eFTLConnection
   * @see eFTLConnection#subscribe
   */
  eFTLConnection.prototype.unsubscribeAll = function() {
    var self = this;
    if (_keys(this.subscriptions).length > 0) {
      _each(this.subscriptions, function(value, key) {
        self.unsubscribe(key);
      });
    }
  };

  /**
   * <p>Gets the client identifier for this connection.</p>
   * @memberOf eFTLConnection
   * @return {string} The unique client identifier.
   * @see eFTL#connect
   */
  eFTLConnection.prototype.getClientId = function() {
    return this.clientId;
  };

  /**
   * @callback onComplete
   * @desc <p>A publish operation has completed successfully.</p>
   *
   * @param {eFTLMessage} message This message has been published.
   * @see eFTLConnection#publish
   */

  /**
   * <p>Publish a one-to-many message to all subscribing clients.</p>
   * <p>This call returns immediately; publishing continues asynchronously.
   * When the publish completes successfully, the eFTL library calls your
   * {@link onComplete} callback.</p>
   * <p>It is good practice to publish each message to a specific
   * destination by using the message field name <code>_dest</code>.
   * @example
   * <caption>To direct a message to a specific destination,
   * add a string field to the message; for example:</caption>
   * message.set("_dest", "myTopic");
   *
   * @memberOf eFTLConnection
   * @param {eFTLMessage} message Publish this message.
   * @param {object} options A JavaScript object holding publish callbacks.
   * @param {onComplete} [options.onComplete] Process publish completion.
   * @param {onError} [options.onError] Process publish errors.
   */
  eFTLConnection.prototype.publish = function(message, options) {
    assertEFTLMessageInstance(message, "Message");
    this._sendMessage(message, options);
  };

  /**
   * Schedules a reconnect attempt
   * @returns {boolean} if it will attempt a reconnect or false if the max number of
   * reconnect tries has been exausted.
   * @private
   */
  eFTLConnection.prototype._scheduleReconnect = function() {
    if (this.reconnectCounter < this.autoReconnectAttempts) {
      var ms = Math.min(Math.pow(2, this.reconnectCounter++) * 1000, this.autoReconnectMaxDelay);
      var connection = this;

      this.reconnectTimer = setTimeout(function() {
        connection.reconnectTimer = null;
        connection.isReconnecting = true;
        connection.open(connection.accessPointURL, connection.connectOptions);
      }, ms);

      return true;
    }

    return false;
  };


  /**
   * <p>Reopen a closed connection.</p>
   *
   * <p>You may call this method within your {@link onDisconnect} callback.</p>
   * <p>This call returns immediately; connecting continues asynchronously. When the connection
   * is ready to use, the eFTL library calls your {@link onReconnect} callback.</p>
   * <p>Reconnecting automatically re-activates all subscriptions on the connection. The eFTL library
   * invokes your {@link onSubscribe} callback for each successful resubscription.
   *
   * @memberOf eFTLConnection
   */
  eFTLConnection.prototype.reconnect = function() {
    if (this.state !== CLOSED) return;
    this.isReconnecting = false;
    this.open(this.accessPointURL, this.connectOptions);
  };

  /**
   * <p>Disconnect from the eFTL server.</p>
   * <p>Programs may disconnect to free server resources.</p>
   * <p>This call returns immediately; disconnecting continues asynchronously.
   * When the connection has closed, the eFTL library calls your {@link onDisconnect} callback.</p>
   *
   * @memberOf eFTLConnection
   * @see onDisconnect
   */
  eFTLConnection.prototype.disconnect = function() {
    this._disconnect();
  };

  // Turns on internal console.logs. Must provide, 'debug: true' on the
  // connect options to enable.
  eFTLConnection.prototype._debug = function(arg) {
    if (this.connectOptions && this.connectOptions.debug) {
      var args = Array.prototype.slice.call(arguments);
      console.log(args);
    }
  };

  // Polyfill for JavaScript implementations that do not support
  // String.prototype.startsWith().
  if (!String.prototype.startsWith) {
    String.prototype.startsWith = function(searchString, position) {
      position = position || 0;
      return this.substr(position, searchString.length) === searchString;
    };
  }

  // Polyfill for JavaScript implementations that do not support
  // String.prototype.trim().
  if (!String.prototype.trim) {
    String.prototype.trim = function() {
      return this.replace(/^[\s\uFEFF\xA0]+|[\s\uFEFF\xA0]+$/g, '');
    };
  }

  // export it to the right root - node or window
  if (typeof exports !== 'undefined') {
    if (typeof module !== 'undefined' && module.exports) {
      exports = module.exports = new eFTL();
    } else {
      exports = new eFTL();
    }
  } else {
    root.eFTL = new eFTL();
    root.eFTLMessage = eFTLMessage;
  }
})();
