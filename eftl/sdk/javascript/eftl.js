/*
 * Copyright (c) $Date: 2022-01-14 14:03:58 -0800 (Fri, 14 Jan 2022) $ TIBCO Software Inc.
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 *
 * $Id: eftl.js 138851 2022-01-14 22:03:58Z $
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

  function _shuffle(array) {
    for (var i = array.length - 1; i > 0; i--) {
      var j = Math.floor(Math.random() * (i + 1));
      var temp = array[i];
      array[i] = array[j];
      array[j] = temp;
    }
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
    } else {
      // browser
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
   * @callback onStateChange
   * @desc <p>The connection state has changed.</p>
   * <p>The eFTL library invokes this method whenever the connection state changes.
   * @param {eFTLConnection} connection The connection whose state has changed.
   * @param {number} state The connection has changed to this state.
   * @see eFTL#connect
   */

  /**
   * <p>Connect to an eFTL server.</p>
   *
   * <p>This call returns immediately; connecting continues asynchronously. When the connection is ready to use,
   * the eFTL library calls your {@link onConnect} callback, passing an <tt>eFTLConnection</tt> object that
   * you can use to publish and subscribe.</p>
   *
   * <p>When a pipe-separated list of URLs is specified this call will attempt a connection to each in turn, 
   * in a random order, until one is connected.</p>
   *
   * <p>A program that uses more than one server channel must connect separately to each channel.</p>
   *
   * @param {string} url The call connects to the eFTL server at this URL. This can be a single URL, or a 
   * pipe ('|') separated list of URLs. URLs can be in either of these forms:
   * <ul>
   *  <li><tt>ws://host:port/channel</tt></li>
   *  <li><tt>wss://host:port/channel</tt></li>
   * </ul>
   * Optionally, URLs can contain the username, password, and/or client identifier:
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
   * @param {number} [options.autoReconnectAttempts] Specify the maximum number of auto-reconnect attempts if the connection is lost.
   * The default value is 256 attempts.
   * @param {number} [options.autoReconnectMaxDelay] Specify the maximum delay (in milliseconds) between auto-reconnect attempts.
   * The default value is 30 seconds.
   * @param {number} [options.maxPendingAcks] Specify the maximum number of unacknowledged messages allowed for the client.
   * Once reached the client will stop receiving additional messages until previously received messages are acknowledged.
   * @param {boolean} [options.trustAll] Specify <code>true</code> to skip eFTL server certificate authentication. 
   * This option should only be used during development and testing. See {@link eFTL#setTrustCertificates}.
   * @param {onConnect} [options.onConnect] A new connection to the eFTL server is ready to use.
   * @param {onError} [options.onError] An error prevented an operation.
   * @param {onDisconnect} [options.onDisconnect] A connection to the eFTL server has closed.
   * @param {onReconnect} [options.onReconnect] A connection to the eFTL server has re-opened and is ready to use.
   * @param {onStateChange} [options.onStateChange] The connection state has changed.
   *
   * @see eFTLConnection
   *
   */
  eFTL.prototype.connect = function(url, options) {
    if (_isUndefined(url) || _isNull(url)) {
      throw new TypeError("The URL is null or undefined.");
    }
    var urls = [];
    _each(url.split("|"), function(u) {
      u = u.trim();
      if (!(u.startsWith('ws:') || u.startsWith('wss:'))) {
        throw new SyntaxError("The URL's scheme must be either 'ws' or 'wss'.");
      }
      urls.push(u);
    });
    // shuffle the URLs
    _shuffle(urls);
    var connection = new eFTLConnection(urls, options);
    connection.open();
  };

  /**
   * Set the certificates to trust when making a secure connection.
   *
   * <p>Self-signed server certificates are not supported.
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
  var EFTL_PROTOCOL_VER = 1;

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
  var OP_REQUEST = 13;
  var OP_REQUEST_REPLY = 14;
  var OP_REPLY = 15;
  var OP_MAP_CREATE = 16;
  var OP_MAP_DESTROY = 18;
  var OP_MAP_SET = 20;
  var OP_MAP_GET = 22;
  var OP_MAP_REMOVE = 24;
  var OP_MAP_RESPONSE = 26;

  var ERR_PUBLISH_DISALLOWED = 12;
  var ERR_PUBLISH_FAILED = 11;
  var ERR_SUBSCRIPTION_DISALLOWED = 13;
  var ERR_SUBSCRIPTION_FAILED = 21;
  var ERR_SUBSCRIPTION_INVALID = 22;
  var ERR_MAP_REQUEST_DISALLOWED = 14;
  var ERR_MAP_REQUEST_FAILED = 30;

  var ERR_TIMEOUT = 99;

  var USER_PROP = "user";
  var USERNAME_PROP = "username";
  var PASSWORD_PROP = "password";
  var CLIENT_ID_PROP = "clientId";
  var AUTO_RECONNECT_ATTEMPTS_PROP = "autoReconnectAttempts";
  var AUTO_RECONNECT_MAX_DELAY_PROP = "autoReconnectMaxDelay";
  var MAX_PENDING_ACKS_PROP = "maxPendingAcks";

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
  var REPLY_TO_FIELD = "reply_to";
  var BODY_FIELD = "body";
  var SEQ_NUM_FIELD = "seq";
  var REQ_ID_FIELD = "req";
  var STORE_MSG_ID_FIELD = "sid";
  var DELIVERY_COUNT_FIELD = "cnt";
  var DOUBLE_FIELD = "_d_";
  var MILLISECOND_FIELD = "_m_";
  var OPAQUE_FIELD = "_o_";
  var LOGIN_OPTIONS_FIELD = "login_options";
  var RESUME_FIELD = "_resume";
  var QOS_FIELD = "_qos";
  var CONNECT_TIMEOUT = "timeout";
  var MAP_FIELD = "map";
  var KEY_FIELD = "key";
  var DEL_FIELD = "del";
  var VALUE_FIELD = "value";
  var PROTOCOL_FIELD = "protocol";
  var MAX_PENDING_ACKS_FIELD = "max_pending_acks";

  var HEADER_PREFIX = "_eftl:"
  var REPLY_TO_HEADER = HEADER_PREFIX + "replyTo";
  var REQUEST_ID_HEADER = HEADER_PREFIX + "requestId";
  var SEQUENCE_NUMBER_HEADER = HEADER_PREFIX + "sequenceNumber";
  var SUBSCRIPTION_ID_HEADER = HEADER_PREFIX + "subscriptionId";
  var STORE_MESSAGE_ID_HEADER = HEADER_PREFIX + "storeMessageId";
  var DELIVERY_COUNT_HEADER = HEADER_PREFIX + "deliveryCount";

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

  eFTLMessage.prototype.setReceipt = function(sequenceNumber, subscriptionId) {
    this[SEQUENCE_NUMBER_HEADER] = sequenceNumber;
    this[SUBSCRIPTION_ID_HEADER] = subscriptionId;
  }

  eFTLMessage.prototype.getReceipt = function() {
    return {
      sequenceNumber: this[SEQUENCE_NUMBER_HEADER],
      subscriptionId: this[SUBSCRIPTION_ID_HEADER],
    };
  }

  eFTLMessage.prototype.setReplyTo = function(replyTo, requestId) {
    this[REPLY_TO_HEADER] = replyTo;
    this[REQUEST_ID_HEADER] = requestId;
  }

  eFTLMessage.prototype.getReplyTo = function() {
    return {
      replyTo: this[REPLY_TO_HEADER],
      requestId: this[REQUEST_ID_HEADER],
    };
  }

  eFTLMessage.prototype.setStoreMessageId = function(msgId) {
    this[STORE_MESSAGE_ID_HEADER] = msgId;
  }

  /**
   * Get the message's unique store identifier assigned
   * by the persistence service.
   *
   * @memberOf eFTLMessage
   * @returns {number} The message identifier.
   */
  eFTLMessage.prototype.getStoreMessageId = function() {
    return this[STORE_MESSAGE_ID_HEADER];
  }

  eFTLMessage.prototype.setDeliveryCount = function(deliveryCount) {
    this[DELIVERY_COUNT_HEADER] = deliveryCount;
  }

  /**
   * Get the message's delivery count assigned
   * by the persistence service.
   *
   * @memberOf eFTLMessage
   * @returns {number} The message delivery count.
   */
  eFTLMessage.prototype.getDeliveryCount = function() {
    return this[DELIVERY_COUNT_HEADER];
  }

  eFTLMessage.prototype.toJSON = function() {
    var result = {};
    for (var k in this) {
      // skip internal headers
      if (!k.startsWith(HEADER_PREFIX)) {
        result[k] = this[k];
      }
    }
    return result;
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
      if (!kn.startsWith(HEADER_PREFIX)) {
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
   * @param {string} name The key-value map name.
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
   * @param {string} key Set the value for this key.
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

    if (connection.state === CONNECTED) {
      connection._send(envelope, sequence, false, data);
    }
  };

  /**
   * <p>Get a key-value pair from the map.</p>
   *
   * @memberOf eFTLKVMap
   * @param {string} key Get the value for this key.
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

    if (connection.state === CONNECTED) {
      connection._send(envelope, sequence, false, data);
    }
  };

  /**
   * <p>Remove a key-value pair from the map.</p>
   *
   * @memberOf eFTLKVMap
   * @param {string} key Remove the value for this key.
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

    if (connection.state === CONNECTED) {
      connection._send(envelope, sequence, false, data);
    }
  };

  /** 
   * @constant 
   * @memberOf eFTL
   */
  var CONNECTING = 0;
  /** 
   * @constant 
   * @memberOf eFTL
   */
  var CONNECTED = 1;
  /** 
   * @constant 
   * @memberOf eFTL
   */
  var RECONNECTING = 2;
  /** 
   * @constant 
   * @memberOf eFTL
   */
  var DISCONNECTING = 3;
  /** 
   * @constant 
   * @memberOf eFTL
   */
  var DISCONNECTED = 4;

  // allow the connection states to be exported through the eFTL object
  [['CONNECTING',CONNECTING],['CONNECTED',CONNECTED],['RECONNECTING',RECONNECTING],['DISCONNECTING',DISCONNECTING],['DISCONNECTED',DISCONNECTED]].forEach(function(property) {
    Object.defineProperty(eFTL.prototype, property[0], {
      get: function() { return property[1]; }
    })
  });

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
  function eFTLConnection(urls, options) {
    this.urlList = urls;
    this.urlIndex = 0;
    this.connectOptions = options;
    this.webSocket = null;
    this.state = DISCONNECTED;
    this.clientId = null;
    this.reconnectToken = null;
    this.protocol = 0;
    this.timeout = 600000;
    this.heartbeat = 240000;
    this.maxMessageSize = 0;
    this.lastMessage = 0;
    this.timeoutCheck = null;
    this.subscriptions = {};
    this.sequenceCounter = 0;
    this.subscriptionCounter = 0;
    this.reconnectCounter = 0;
    this.autoReconnectAttempts = 256;
    this.autoReconnectMaxDelay = 30000;
    this.reconnectTimer = null;
    this.connectTimer = null;
    this.isReconnecting = false;
    this.isOpen = false;
    this.qos = true;
    this.requests = {};
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

  eFTLConnection.prototype._setState = function(state) {
    if (this.state != state) {
        this.state = state;
        this._caller(this.connectOptions.onStateChange, this.connectOptions, [this, this.state]);
    }
  }

  /**
   * Determine whether this connection to the eFTL server is open or closed.
   * @memberOf eFTLConnection
   * @returns {boolean} <tt>true</tt> if this connection is open; <tt>false</tt> otherwise.
   */
  eFTLConnection.prototype.isConnected = function() {
    return this.state === CONNECTED;
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

  /**
   * <p>Remove a key-value map.</p>
   *
   * @memberOf eFTLConnection
   * @param {string} name The key-value map name.
   * @see eFTLConnection#createKVMap
   */
  eFTLConnection.prototype.removeKVMap = function(name) {
    var envelope = {};
    envelope[OP_FIELD] = OP_MAP_DESTROY;
    envelope[MAP_FIELD] = name;
    this._send(envelope, 0, false);
  };

  eFTLConnection.prototype._send = function(message, seqNum, forceSend) {
    // the 4th argument provided by a call is already stringified - used by heartbeats
    var data = arguments.length === 4 ? arguments[3] : JSON.stringify(message);

    this._debug('>>', message);

    if (this.webSocket) {
      if (this.state < DISCONNECTING || forceSend) {
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
    subscription.lastSeqNum = 0;
    subscription.autoAck = true;

    if (options["ack"] !== undefined) {
      subscription.autoAck = (options["ack"] === "auto");
    }

    var subMessage = {};
    subMessage[OP_FIELD] = OP_SUBSCRIBE;
    subMessage[ID_FIELD] = subId;
    subMessage[MATCHER_FIELD] = options.matcher;
    subMessage[DURABLE_FIELD] = options.durable;

    //include options
    var opts = _safeExtend({}, options);
    delete opts["matcher"];
    delete opts["durable"];
    _extend(subMessage, opts);

    this.subscriptions[subId] = subscription;
    this._send(subMessage, 0, false);

    return subId;
  };

  eFTLConnection.prototype._acknowledge = function(sequenceNumber, subscriptionId) {
    if (!this.qos || !sequenceNumber) {
      return;
    }

    var envelope = {};
    envelope[OP_FIELD] = OP_ACK;
    envelope[SEQ_NUM_FIELD] = sequenceNumber;
    if (subscriptionId !== undefined) {
        envelope[ID_FIELD] = subscriptionId;
    }
    this._send(envelope, 0, false);
  };

  eFTLConnection.prototype._sendMessage = function(message, op, to, reqId, timeout, options) {
    var sequence = this._nextSequence();

    var envelope = {};
    envelope[OP_FIELD] = op;
    envelope[SEQ_NUM_FIELD] = sequence;
    envelope[BODY_FIELD] = new eFTLMessage(message);

    if (to) {
      envelope[TO_FIELD] = to;
      envelope[REQ_ID_FIELD] = reqId;
    }

    var publish = {};
    publish.options = options;
    publish.message = message;
    publish.envelope = envelope;
    publish.sequence = sequence;

    var data = JSON.stringify(envelope);

    if (this.maxMessageSize > 0 && data.length > this.maxMessageSize)
      throw new Error("Message exceeds maximum message size of " + this.maxMessageSize);

    if (timeout > 0) {
      var connection = this;
      publish.timer = setTimeout(function() {
        connection._pendingError(sequence, ERR_TIMEOUT, "request timeout");
      }, timeout);
    }

    this.requests[sequence] = publish;

    if (this.state === CONNECTED) {
      this._send(envelope, sequence, false, data);
    }

    return publish;
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
    if (this.state != CONNECTED &&
        this.state != CONNECTING &&
        this.state != RECONNECTING) {
        return;
    }

    this._setState(DISCONNECTING);

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

    // remove the subscription only if it's untryable
    if (errCode == ERR_SUBSCRIPTION_INVALID) {
      delete this.subscriptions[subId];
    }

    if (sub != null) {
      sub.pending = true;
      var options = sub.options;
      this._caller(options.onError, options, [subId, errCode, reason]);
    }
  };

  eFTLConnection.prototype._handleEvent = function(message) {
    var subId = message[TO_FIELD];
    var seqNum = message[SEQ_NUM_FIELD];
    var msgId = message[STORE_MSG_ID_FIELD];
    var deliveryCount = message[DELIVERY_COUNT_FIELD];
    var reqId = message[REQ_ID_FIELD];
    var replyTo = message[REPLY_TO_FIELD];
    var data = message[BODY_FIELD];
    var sub = this.subscriptions[subId];

    if (sub && sub.options != null) {
      if (!seqNum || seqNum > sub.lastSeqNum) {
        var wsMessage = new eFTLMessage(data);
        if (!sub.autoAck && seqNum) {
          wsMessage.setReceipt(seqNum, subId);
        }
        if (replyTo) {
          wsMessage.setReplyTo(replyTo, reqId);
        }
        wsMessage.setStoreMessageId(msgId ? msgId : 0);
        wsMessage.setDeliveryCount(deliveryCount ? deliveryCount : 0);
        this._debug('<<', data);
        this._caller(sub.options.onMessage, sub.options, [wsMessage]);
        // track last sequence number
        if (sub.autoAck && seqNum) {
          sub.lastSeqNum = seqNum;
        }
      }
      // ack last sequence number, server will ack all messages less or equal
      if (sub.autoAck && seqNum)
        this._acknowledge(seqNum, subId);
    }
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
      var url = parseURL(connection._getURL());

      var auth = (url.auth ? url.auth.split(':') : []);
      var username = auth[0];
      var password = auth[1];
      var clientId = getQueryVariable(url, "clientId");

      var loginMessage = {};

      loginMessage[OP_FIELD] = OP_LOGIN;
      loginMessage[PROTOCOL_FIELD] = EFTL_PROTOCOL_VER;
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

      if (_isNumber(connection.connectOptions[MAX_PENDING_ACKS_PROP])) {
        var maxPendingAcks = connection.connectOptions[MAX_PENDING_ACKS_PROP];
        if (maxPendingAcks > 0) {
            loginMessage[MAX_PENDING_ACKS_FIELD] = connection.connectOptions[MAX_PENDING_ACKS_PROP];
        }
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

      if (connection.state === DISCONNECTED) return;

      connection._setState(DISCONNECTED);

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

      // Reconnect when the close code reflects an unexpected error (1006) or a server restart (1012).
      if ((evt.code != 1006 && evt.code != 1012) || !connection._scheduleReconnect()) {
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
              case OP_REQUEST_REPLY:
                connection._handleReply(msg);
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
    this.protocol = (message[PROTOCOL_FIELD] || 0);
    this.qos = ((message[QOS_FIELD] || "") + "").toLowerCase() === "true";

    if (!resume) {
      this._clearPending(11, "Reconnect");
      this.lastSequenceNumber = 0;
    }

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

    this._setState(CONNECTED);
    this.reconnectCounter = 0;

    // rest URL list to start auto-reconnect attempts from the beginning
    this._resetURLList();

    // restore the subscriptions
    if (_keys(this.subscriptions).length > 0) {
      _each(this.subscriptions, function(value, key) {
        if (!resume) {
            value.lastSeqNum = 0;
        }
        connection._subscribe(key, value.options);
      });
    }

    // re-send unacknowledged messages
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

    if (this.state === CONNECTING || this.state === RECONNECTING) {
      this._close(reason, false);
    }
  };

  eFTLConnection.prototype._handleAck = function(message) {
    var sequence = message[SEQ_NUM_FIELD];
    var errCode = message[ERR_CODE_FIELD];
    var reason = message[REASON_FIELD];
    if (!_isUndefined(sequence)) {
      if (_isUndefined(errCode) || _isNull(errCode)) {
        this._pendingComplete(sequence);
      } else {
        this._pendingError(sequence, errCode, reason);
      }
    }
  };

  eFTLConnection.prototype._handleReply = function(message) {
    var sequence = message[SEQ_NUM_FIELD];
    var body = message[BODY_FIELD];
    var errCode = message[ERR_CODE_FIELD];
    var reason = message[REASON_FIELD];
    if (!_isUndefined(sequence)) {
      if (_isUndefined(errCode) || _isNull(errCode)) {
        if (_isUndefined(body) || _isNull(body)) {
          this._pendingComplete(sequence);
        } else {
          this._pendingResponse(sequence, new eFTLMessage(body));
        }
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
      if (req.timer) {
          clearTimeout(req.timer);
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
      if (req.timer) {
          clearTimeout(req.timer);
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
      if (req.timer) {
          clearTimeout(req.timer);
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

  eFTLConnection.prototype._resetURLList = function() {
    this.urlIndex = 0;
  };

  eFTLConnection.prototype._nextURL = function() {
    if (++this.urlIndex < this.urlList.length) {
      return true;
    } else {
      this.urlIndex = 0;
      return false;
    }
  };

  eFTLConnection.prototype._getURL = function() {
    return this.urlList[this.urlIndex];
  };

  eFTLConnection.prototype.open = function() {
    if (this.isReconnecting) {
        this._setState(RECONNECTING);
    } else {
        this._setState(CONNECTING);
    }

    try {
      var options = {};

      if (trustCertificates !== undefined) {
        options.ca = trustCertificates;
      }

      if (_isBoolean(this.connectOptions.trustAll) && this.connectOptions.trustAll) {
        options.rejectUnauthorized = false;
      }

      var url = parseURL(this._getURL());

      var auth = (url.auth ? url.auth.split(':') : []);
      var username = auth[0];
      var password = auth[1];
      var clientId = getQueryVariable(url, "clientId");

      if (_isUndefined(username)) {
        if (this.connectOptions.hasOwnProperty(USERNAME_PROP)) {
          username = this.connectOptions[USERNAME_PROP];
        } else {
          username = this.connectOptions[USER_PROP];
        }
      }

      if (_isUndefined(password)) {
        password = this.connectOptions[PASSWORD_PROP];
      }

      if (_isUndefined(clientId)) {
        clientId = this.connectOptions[CLIENT_ID_PROP];
      }

      var urlString = "";

      if (_isString(username) || _isString(password)) {
        urlString = url.protocol + "//" + (username || "") + ":"
                    + (password || "") + "@" + url.host + url.pathname;
      } else {
        urlString = url.protocol + "//" + url.host + url.pathname;
      }

      if (_isString(clientId) && clientId != "") {
        urlString = urlString + "?client_id=" + clientId;
      }

      this.webSocket = new WebSocket(urlString, [EFTL_WS_PROTOCOL], options);
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
   * <p>To close the subscription, call {@link eFTLConnection#unsubscribe} or 
   * {@link eFTLConnection#closeSubscription} with the subscription identifier.</p>
   *
   * @param {object} subscriptionId The subscription identifier.
   * @see eFTLConnection#subscribe
   * @see eFTLConnection#unsubscribe
   * @see eFTLConnection#closeSubscription
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
   * <p>
   * An acknowledgment mode for the subscription can be set to automatically acknowledge consumed
   * messages, require explicit client acknowledgment of the consumed messages, or to disable
   * message acknowledgment altogether. The default is to automatically acknowledge consumed messages.
   * <p>
   * When explicit client acknowledgment is specified the eFTL server will stop delivering messages 
   * to the client once the server's configured maximum number of unacknowledged messages is reached.
   * <p>
   * When communicating with EMS, to subscribe to messages published on a specific EMS
   * destination use a subscription matcher that includes the message field name <code>_dest</code>.
   * To distinguish between topics and queues the destination name can be prefixed with
   * either "TOPIC:" or "QUEUE:", for example "TOPIC:MyDest" or "QUEUE:MyDest". A destination name
   * with no prefix is a topic.
   * @example
   * <caption>To subscribe to messages published on a specific EMS destination,
   * create a subscription matcher for that destination;
   * for example:</caption>
   * var matcher = '{"_dest":"MyDest"}';
   *
   * @memberOf eFTLConnection
   * @param {object} options A JavaScript object holding subscription callbacks.
   *
   * @param {string} [options.matcher] The subscription uses this content matcher to narrow the message stream.
   * @param {string} [options.durable] The subscription is associated with this durable name.
   * @param {string} [options.ack] An optional message acknowledgment mode; 'auto', 'client', or 'none'. Default is 'auto'.
   * @param {string} [options.type] An optional durable type; 'shared' or 'last-value'.
   * @param {string} [options.key] The key field for 'last-value' durable subscriptions.
   * @param {onSubscribe} [options.onSubscribe] The new subscription is ready.
   * @param {onError} [options.onError] Process subscription errors.
   * @param {onMessage} [options.onMessage] Process inbound messages
   * @returns {object} The subscription identifier.
   *
   * @see eFTLConnection#unsubscribe
   * @see eFTLConnection#closeSubscription
   * @see eFTLConnection#acknowledge
   * @see eFTLConnection#acknowledgeAll
   */
  eFTLConnection.prototype.subscribe = function(options) {
    var subId = this._nextSubscriptionSequence().toString(10);
    this._subscribe(subId, options);
    return subId;
  };

  /**
   * <p>Close a subscription.</p>
   * <p>
   * For durable subscriptions, this call will cause the persistence
   * service to stop delivering messages while leaving the durable 
   * subscription to continue accumulating messages. Any unacknowledged
   * messages will be made available for redelivery.
   * <p>Programs receive subscription identifiers via their {@link onSubscribe} handlers.</p>
   * @memberOf eFTLConnection
   * @param {object} subscriptionId Close this subscription.
   * @see eFTLConnection#subscribe
   */
  eFTLConnection.prototype.closeSubscription = function(subscriptionId) {
    if (this.protocol < 1) {
      throw new Error("close subscriptions is not supported with this server");
    }

    var unsubMessage = {};
    unsubMessage[OP_FIELD] = OP_UNSUBSCRIBE;
    unsubMessage[ID_FIELD] = subscriptionId;
    unsubMessage[DEL_FIELD] = false;

    this._send(unsubMessage, 0, false);
    delete this.subscriptions[subscriptionId];
  };

  /**
   * <p>Close all subscriptions.</p>
   * <p>
   * For durable subscriptions, this call will cause the persistence
   * service to stop delivering messages while leaving the durable 
   * subscriptions to continue accumulating messages. Any unacknowledged
   * messages will be made available for redelivery.
   * @memberOf eFTLConnection
   * @see eFTLConnection#subscribe
   */
  eFTLConnection.prototype.closeAllSubscriptions = function() {
    var self = this;
    if (_keys(this.subscriptions).length > 0) {
      _each(this.subscriptions, function(value, key) {
        self.closeSubscription(key);
      });
    }
  };

  /**
   * <p>Unsubscribe from messages on a subscription.</p>
   * <p>
   * For durable subscriptions, this call will cause the persistence
   * service to remove the durable subscription, along with any
   * persisted messages. 
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
   * <p>Unsubscribe from messages on all subscriptions.</p>
   * <p>
   * For durable subscriptions, this call will cause the persistence
   * service to remove the durable subscription, along with any
   * persisted messages. 
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
   * <p>Acknowledge this message.</p>
   * <p>
   * Messages consumed from subscriptions with a client acknowledgment mode
   * must be explicitly acknowledged. The eFTL server will stop delivering
   * messages to the client once the server's configured maximum number of
   * unacknowledged messages is reached.
   * </p>
   * @memberOf eFTLConnection
   * @param {object} message The message being acknowledged.
   * @see eFTLConnection#acknowledgeAll
   */
  eFTLConnection.prototype.acknowledge = function(message) {
    var receipt = message.getReceipt();
    this._acknowledge(receipt.sequenceNumber);
  };

  /**
   * <p>Acknowledge all messages up to and including this message.</p>
   * <p>
   * Messages consumed from subscriptions with a client acknowledgment mode
   * must be explicitly acknowledged. The eFTL server will stop delivering
   * messages to the client once the server's configured maximum number of
   * unacknowledged messages is reached.
   * </p>
   * @memberOf eFTLConnection
   * @param {object} message The message being acknowledged.
   * @see eFTLConnection#acknowledge
   */
  eFTLConnection.prototype.acknowledgeAll = function(message) {
    var receipt = message.getReceipt();
    this._acknowledge(receipt.sequenceNumber, receipt.subscriptionId);
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
   * @callback onReply
   * @desc <p>A request operation has received a reply.</p>
   *
   * @param {eFTLMessage} reply The reply message.
   * @see eFTLConnection#sendRequest
   */

  /**
   * <p>Publish a one-to-many message to all subscribing clients.</p>
   * <p>This call returns immediately; publishing continues asynchronously.
   * When the publish completes successfully, the eFTL library calls your
   * {@link onComplete} callback.</p>
   * <p>
   * When communicating with EMS, to publish a messages on a specific EMS
   * destination include the message field name <code>_dest</code>.
   * To distinguish between topics and queues the destination can be prefixed with
   * either "TOPIC:" or "QUEUE:", for example "TOPIC:MyDest" or "QUEUE:MyDest". A 
   * destination with no prefix is a topic.
   * @example
   * <caption>To publish a message on a specific EMS destination
   * add a string field to the message; for example:</caption>
   * message.set("_dest", "MyDest");
   *
   * @memberOf eFTLConnection
   * @param {eFTLMessage} message Publish this message.
   * @param {object} options A JavaScript object holding publish callbacks.
   * @param {onComplete} [options.onComplete] Process publish completion.
   * @param {onError} [options.onError] Process publish errors.
   */
  eFTLConnection.prototype.publish = function(message, options) {
    assertEFTLMessageInstance(message, "Message");
    this._sendMessage(message, OP_MESSAGE, null, 0, 0, options);
  };

  /**
   * <p>Publish a request message.</p>
   * <p>This call returns immediately. When the reply is received
   * the eFTL library calls your {@link onReply} callback.</p>
   *
   * @memberOf eFTLConnection
   * @param {eFTLMessage} request The request message to publish.
   * @param {number} timeout The number of milliseconds to wait for a reply.
   * @param {object} options A JavaScript object holding request callbacks.
   * @param {onReply} [options.onReply] Process reply.
   * @param {onError} [options.onError] Process request errors.
   */
  eFTLConnection.prototype.sendRequest = function(request, timeout, options) {
    assertEFTLMessageInstance(request, "Message");
    if (this.protocol < 1) {
      throw new Error("Send request is not supported with this server");
    }
    if (options) {
      options.onComplete = options.onReply
    }
    this._sendMessage(request, OP_REQUEST, null, 0, timeout, options);
  };

  /**
   * <p>Send a reply message in response to a request message.</p>
   * <p>This call returns immediately. When the send completes
   * successfully, the eFTL library calls your {@link onComplete} callback.</p>
   *
   * @memberOf eFTLConnection
   * @param {eFTLMessage} reply The reply message to send.
   * @param {eFTLMessage} request The request message.
   * @param {object} options A JavaScript object holding request callbacks.
   * @param {onComplete} [options.onComplete] Process reply completion.
   * @param {onError} [options.onError] Process request errors.
   */
  eFTLConnection.prototype.sendReply = function(reply, request, options) {
    assertEFTLMessageInstance(request, "Message");
    if (this.protocol < 1) {
      throw new Error("Send reply is not supported with this server");
    }
    var replyTo = request.getReplyTo();
    if (!replyTo.replyTo) {
      throw new TypeError("Not a request message.");
    }
    this._sendMessage(reply, OP_REPLY, replyTo.replyTo, replyTo.requestId, 0, options);
  };

  /**
   * Schedules a reconnect attempt
   * @returns {boolean} if it will attempt a reconnect or false if the max number of
   * reconnect tries has been exausted.
   * @private
   */
  eFTLConnection.prototype._scheduleReconnect = function() {
    // schedule a connect only if not previously connected
    // and there are additional URLs to try in the URL list
    //
    // schedule a reconnect only if previously connected and
    // the number of reconnect attempts has not been exceeded
    if (!this.isOpen && this._nextURL()) {
      var connection = this;

      this.reconnectTimer = setTimeout(function() {
        connection.reconnectTimer = null;
        connection.isReconnecting = false;
        connection.open();
      }, 0);

      return true;
    } else if (this.isOpen && this.reconnectCounter < this.autoReconnectAttempts) {
      var ms = 0;

      // backoff once all URLs have been tried
      if (!this._nextURL()) {
          // add jitter by applying a randomness factor of 0.5
          var jitter = Math.random() + 0.5; 
          var backoff = Math.pow(2, this.reconnectCounter++) * 1000 * jitter;
          if (backoff > this.autoReconnectMaxDelay || backoff <= 0) 
		backoff = this.autoReconnectMaxDelay;
          ms = backoff;
      }

      var connection = this;

      this.reconnectTimer = setTimeout(function() {
        connection.reconnectTimer = null;
        connection.isReconnecting = true;
        connection.open();
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
    if (this.state !== DISCONNECTED) return;
    this.isReconnecting = false;
    this.open();
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
