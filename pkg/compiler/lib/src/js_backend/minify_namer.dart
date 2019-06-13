// Copyright (c) 2011, the Dart project authors.  Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.

part of js_backend.namer;

/// Assigns JavaScript identifiers to Dart variables, class-names and members.
class MinifyNamer extends Namer
    with
        _MinifiedFieldNamer,
        _MinifyConstructorBodyNamer,
        _MinifiedOneShotInterceptorNamer {
  MinifyNamer(
      JClosedWorld closedWorld, RuntimeTypeTags rtiTags, FixedNames fixedNames)
      : super(closedWorld, rtiTags, fixedNames) {
    reserveBackendNames();
    fieldRegistry = new _FieldNamingRegistry(this);
  }

  @override
  _FieldNamingRegistry fieldRegistry;

  @override
  String get isolateName => 'I';
  @override
  String get isolatePropertiesName => 'p';
  @override
  bool get shouldMinify => true;
  @override
  String get genericInstantiationPrefix => r'$I';

  final ALPHABET_CHARACTERS = 52; // a-zA-Z.
  final ALPHANUMERIC_CHARACTERS = 62; // a-zA-Z0-9.

  /// You can pass an invalid identifier to this and unlike its non-minifying
  /// counterpart it will never return the proposedName as the new fresh name.
  ///
  /// [sanitizeForNatives] and [sanitizeForAnnotations] are ignored because the
  /// minified names will always avoid clashing with annotated names or natives.
  @override
  String _generateFreshStringForName(String proposedName, NamingScope scope,
      {bool sanitizeForNatives: false, bool sanitizeForAnnotations: false}) {
    String freshName;
    String suggestion = scope.suggestName(proposedName);
    if (suggestion != null && scope.isUnused(suggestion)) {
      freshName = suggestion;
    } else {
      freshName = _getUnusedName(proposedName, scope);
    }
    scope.registerUse(freshName);
    return freshName;
  }

  // From issue 7554.  These should not be used on objects (as instance
  // variables) because they clash with names from the DOM. However, it is
  // OK to use them as fields, as we only access fields directly if we know
  // the receiver type.
  static const List<String> _reservedNativeProperties = const <String>[
    'a', 'b', 'c', 'd', 'e', 'f', 'r', 'x', 'y', 'z', 'Q',
    // 2-letter:
    'ch', 'cx', 'cy', 'db', 'dx', 'dy', 'fr', 'fx', 'fy', 'go', 'id', 'k1',
    'k2', 'k3', 'k4', 'r1', 'r2', 'rx', 'ry', 'x1', 'x2', 'y1', 'y2',
    // 3-letter:
    'add', 'all', 'alt', 'arc', 'CCW', 'cmp', 'dir', 'end', 'get', 'in1',
    'in2', 'INT', 'key', 'log', 'low', 'm11', 'm12', 'm13', 'm14', 'm21',
    'm22', 'm23', 'm24', 'm31', 'm32', 'm33', 'm34', 'm41', 'm42', 'm43',
    'm44', 'max', 'min', 'now', 'ONE', 'put', 'red', 'rel', 'rev', 'RGB',
    'sdp', 'set', 'src', 'tag', 'top', 'uid', 'uri', 'url', 'URL',
    // 4-letter:
    'abbr', 'atob', 'Attr', 'axes', 'axis', 'back', 'BACK', 'beta', 'bias',
    'Blob', 'blue', 'blur', 'BLUR', 'body', 'BOOL', 'BOTH', 'btoa', 'BYTE',
    'cite', 'clip', 'code', 'cols', 'cues', 'data', 'DECR', 'DONE', 'face',
    'file', 'File', 'fill', 'find', 'font', 'form', 'gain', 'hash', 'head',
    'high', 'hint', 'host', 'href', 'HRTF', 'IDLE', 'INCR', 'info', 'INIT',
    'isId', 'item', 'KEEP', 'kind', 'knee', 'lang', 'left', 'LESS', 'line',
    'link', 'list', 'load', 'loop', 'mode', 'name', 'Node', 'None', 'NONE',
    'only', 'open', 'OPEN', 'ping', 'play', 'port', 'rect', 'Rect', 'refX',
    'refY', 'RGBA', 'root', 'rows', 'save', 'seed', 'seek', 'self', 'send',
    'show', 'SINE', 'size', 'span', 'stat', 'step', 'stop', 'tags', 'text',
    'Text', 'time', 'type', 'view', 'warn', 'wrap', 'ZERO'
  ];

  void reserveBackendNames() {
    for (String name in _reservedNativeProperties) {
      if (name.length < 2) {
        // Ensure the 1-letter names are disambiguated to the same name.
        String disambiguatedName = name;
        reservePublicMemberName(name, disambiguatedName);
      }
      instanceScope.registerUse(name);
      // Getter and setter names are autogenerated by prepending 'g' and 's' to
      // field names.  Therefore there are some field names we don't want to
      // use.  It is implicit in the next line that the banned prefix is
      // only one character.
      if (_hasBannedPrefix(name)) {
        instanceScope.registerUse(name.substring(1));
      }
    }

    // These popular names are present in most programs and deserve
    // single character minified names.  We could determine the popular names
    // individually per program, but that would mean that the output of the
    // minifier was less stable from version to version of the program being
    // minified.
    _populateSuggestedNames(instanceScope, const <String>[
      r'$add',
      r'add$1',
      r'$and',
      r'$or',
      r'current',
      r'$shr',
      r'$eq',
      r'$ne',
      r'$index',
      r'$indexSet',
      r'$xor',
      r'clone$0',
      r'iterator',
      r'length',
      r'$lt',
      r'$gt',
      r'$le',
      r'$ge',
      r'moveNext$0',
      r'node',
      r'on',
      r'$negate',
      r'push',
      r'self',
      r'start',
      r'target',
      r'$shl',
      r'value',
      r'width',
      r'style',
      r'noSuchMethod$1',
      r'$mul',
      r'$div',
      r'$sub',
      r'$not',
      r'$mod',
      r'$tdiv',
      r'toString$0'
    ]);

    _populateSuggestedNames(globalScope, const <String>[
      r'Object',
      'wrapException',
      r'$eq',
      r'S',
      r'ioore',
      r'UnsupportedError$',
      r'length',
      r'$sub',
      r'$add',
      r'$gt',
      r'$ge',
      r'$lt',
      r'$le',
      r'add',
      r'iae',
      r'ArgumentError$',
      r'BoundClosure',
      r'Closure',
      r'StateError$',
      r'getInterceptor$',
      r'max',
      r'$mul',
      r'Map',
      r'Key_Key',
      r'$div',
      r'List_List$from',
      r'LinkedHashMap_LinkedHashMap$_empty',
      r'LinkedHashMap_LinkedHashMap$_literal',
      r'min',
      r'RangeError$value',
      r'JSString',
      r'JSNumber',
      r'JSArray',
      r'createInvocationMirror',
      r'String',
      r'setRuntimeTypeInfo',
      r'createRuntimeType'
    ]);
  }

  void _populateSuggestedNames(NamingScope scope, List<String> suggestions) {
    int c = $a - 1;
    String letter;
    for (String name in suggestions) {
      do {
        assert(c != $Z);
        c = (c == $z) ? $A : c + 1;
        letter = new String.fromCharCodes([c]);
      } while (_hasBannedPrefix(letter) || scope.isUsed(letter));
      assert(!scope.hasSuggestion(name));
      scope.addSuggestion(name, letter);
    }
  }

  // This gets a minified name based on a hash of the proposed name.  This
  // is slightly less efficient than just getting the next name in a series,
  // but it means that small changes in the input program will give smallish
  // changes in the output, which can be useful for diffing etc.
  String _getUnusedName(String proposedName, NamingScope scope) {
    int hash = _calculateHash(proposedName);
    // Avoid very small hashes that won't try many names.
    hash = hash < 1000 ? hash * 314159 : hash; // Yes, it's prime.

    // Try other n-character names based on the hash.  We try one to three
    // character identifiers.  For each length we try around 10 different names
    // in a predictable order determined by the proposed name.  This is in order
    // to make the renamer stable: small changes in the input should nornally
    // result in relatively small changes in the output.
    for (int n = 1; n <= 3; n++) {
      int h = hash;
      while (h > 10) {
        List<int> codes = <int>[_letterNumber(h)];
        int h2 = h ~/ ALPHABET_CHARACTERS;
        for (int i = 1; i < n; i++) {
          codes.add(_alphaNumericNumber(h2));
          h2 ~/= ALPHANUMERIC_CHARACTERS;
        }
        final candidate = new String.fromCharCodes(codes);
        if (scope.isUnused(candidate) &&
            !jsReserved.contains(candidate) &&
            !_hasBannedPrefix(candidate) &&
            (n != 1 || scope.isSuggestion(candidate))) {
          return candidate;
        }
        // Try again with a slightly different hash.  After around 10 turns
        // around this loop h is zero and we try a longer name.
        h ~/= 7;
      }
    }
    return _badName(hash, scope);
  }

  /// Instance members starting with g and s are reserved for getters and
  /// setters.
  static bool _hasBannedPrefix(String name) {
    int code = name.codeUnitAt(0);
    return code == $g || code == $s;
  }

  int _calculateHash(String name) {
    int h = 0;
    for (int i = 0; i < name.length; i++) {
      h += name.codeUnitAt(i);
      h &= 0xffffffff;
      h += h << 10;
      h &= 0xffffffff;
      h ^= h >> 6;
      h &= 0xffffffff;
    }
    return h;
  }

  /// Remember bad hashes to avoid using a the same character with long numbers
  /// for frequent hashes. For example, `closure` is a very common name.
  Map<int, int> _badNames = new Map<int, int>();

  /// If we can't find a hash based name in the three-letter space, then base
  /// the name on a letter and a counter.
  String _badName(int hash, NamingScope scope) {
    int count = _badNames.putIfAbsent(hash, () => 0);
    String startLetter =
        new String.fromCharCodes([_letterNumber(hash + count)]);
    _badNames[hash] = count + 1;
    String name;
    int i = 0;
    do {
      name = "$startLetter${i++}";
    } while (scope.isUsed(name));
    // We don't need to check for banned prefix because the name is in the form
    // xnnn, where nnn is a number.  There can be no getter or setter called
    // gnnn since that would imply a numeric field name.
    return name;
  }

  int _letterNumber(int x) {
    if (x >= ALPHABET_CHARACTERS) x %= ALPHABET_CHARACTERS;
    if (x < 26) return $a + x;
    return $A + x - 26;
  }

  int _alphaNumericNumber(int x) {
    if (x >= ALPHANUMERIC_CHARACTERS) x %= ALPHANUMERIC_CHARACTERS;
    if (x < 26) return $a + x;
    if (x < 52) return $A + x - 26;
    return $0 + x - 52;
  }

  @override
  jsAst.Name instanceFieldPropertyName(FieldEntity element) {
    jsAst.Name proposed = _minifiedInstanceFieldPropertyName(element);
    if (proposed != null) {
      return proposed;
    }
    return super.instanceFieldPropertyName(element);
  }
}

/// Implements naming for constructor bodies.
///
/// Constructor bodies are only called in settings where the target is
/// statically known. Therefore, we can share their names between classes.
/// However, to support calling the constructor body of a super constructor,
/// each level in the inheritance tree has to use its own names.
///
/// This class implements a naming scheme by counting the distance from
/// a given constructor to [Object], where distance is the number of
/// constructors declared along the inheritance chain.
class _ConstructorBodyNamingScope {
  final int _startIndex;
  final List<ConstructorEntity> _constructors;
  int get numberOfConstructors => _constructors.length;

  _ConstructorBodyNamingScope.rootScope(
      ClassEntity cls, ElementEnvironment environment)
      : _startIndex = 0,
        _constructors = _getConstructorList(cls, environment);

  _ConstructorBodyNamingScope.forClass(ClassEntity cls,
      _ConstructorBodyNamingScope superScope, ElementEnvironment environment)
      : _startIndex = superScope._startIndex + superScope.numberOfConstructors,
        _constructors = _getConstructorList(cls, environment);

  // Mixin Applications have constructors but we never generate code for them,
  // so they do not count in the inheritance chain.
  _ConstructorBodyNamingScope.forMixinApplication(
      ClassEntity cls, _ConstructorBodyNamingScope superScope)
      : _startIndex = superScope._startIndex + superScope.numberOfConstructors,
        _constructors = const [];

  factory _ConstructorBodyNamingScope(
      ClassEntity cls,
      Map<ClassEntity, _ConstructorBodyNamingScope> registry,
      ElementEnvironment environment) {
    return registry.putIfAbsent(cls, () {
      ClassEntity superclass = environment.getSuperClass(cls);
      if (superclass == null) {
        return new _ConstructorBodyNamingScope.rootScope(cls, environment);
      } else if (environment.isMixinApplication(cls)) {
        return new _ConstructorBodyNamingScope.forMixinApplication(cls,
            new _ConstructorBodyNamingScope(superclass, registry, environment));
      } else {
        return new _ConstructorBodyNamingScope.forClass(
            cls,
            new _ConstructorBodyNamingScope(superclass, registry, environment),
            environment);
      }
    });
  }

  String constructorBodyKeyFor(ConstructorBodyEntity body) {
    int position = _constructors.indexOf(body.constructor);
    assert(position >= 0, failedAt(body, "constructor body missing"));
    return "@constructorBody@${_startIndex + position}";
  }

  static List<ConstructorEntity> _getConstructorList(
      ClassEntity cls, ElementEnvironment environment) {
    var result = <ConstructorEntity>[];
    environment.forEachConstructor(cls, result.add);
    return result;
  }
}

abstract class _MinifyConstructorBodyNamer implements Namer {
  Map<ClassEntity, _ConstructorBodyNamingScope> _constructorBodyScopes =
      new Map<ClassEntity, _ConstructorBodyNamingScope>();

  @override
  jsAst.Name constructorBodyName(ConstructorBodyEntity method) {
    _ConstructorBodyNamingScope scope = new _ConstructorBodyNamingScope(
        method.enclosingClass, _constructorBodyScopes, _elementEnvironment);
    String key = scope.constructorBodyKeyFor(method);
    return _disambiguateMemberByKey(
        key, () => _proposeNameForConstructorBody(method));
  }
}

abstract class _MinifiedOneShotInterceptorNamer implements Namer {
  /// Property name used for the one-shot interceptor method for the given
  /// [selector] and return-type specialization.
  @override
  jsAst.Name nameForOneShotInterceptor(
      Selector selector, Iterable<ClassEntity> classes) {
    String root = selector.isOperator
        ? operatorNameToIdentifier(selector.name)
        : privateName(selector.memberName);
    String prefix =
        selector.isGetter ? r"$get" : selector.isSetter ? r"$set" : "";
    String callSuffix = selector.isCall
        ? Namer.callSuffixForStructure(selector.callStructure).join()
        : "";
    String suffix =
        suffixForGetInterceptor(_commonElements, _nativeData, classes);
    String fullName = "\$intercepted$prefix\$$root$callSuffix\$$suffix";
    return _disambiguateInternalGlobal(fullName);
  }
}
