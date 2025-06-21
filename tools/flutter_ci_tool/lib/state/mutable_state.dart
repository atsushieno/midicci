import 'package:flutter/foundation.dart';

/// Flutter equivalent of C++ MutableState<T>
/// Provides reactive state management with change notifications
class MutableState<T> extends ValueNotifier<T> {
  MutableState(super.initialValue);
  
  /// Set new value and notify listeners if changed
  void set(T newValue) {
    if (value != newValue) {
      value = newValue;
    }
  }
  
  /// Get current value (same as .value but matches C++ API)
  T get() => value;
  
  /// Add a value changed handler (callback)
  void setValueChangedHandler(VoidCallback handler) {
    addListener(handler);
  }
  
  /// Remove a value changed handler
  void removeValueChangedHandler(VoidCallback handler) {
    removeListener(handler);
  }
}

/// State change actions for MutableStateList
enum StateChangeAction {
  added,
  removed,
}

/// Flutter equivalent of C++ MutableStateList<T>  
/// Provides reactive list management with change notifications
class MutableStateList<T> extends ValueNotifier<List<T>> {
  void Function(StateChangeAction action, T item)? _collectionChangedHandler;
  
  MutableStateList() : super([]);
  
  /// Get unmodifiable view of items
  List<T> get items => List.unmodifiable(value);
  
  /// Get number of items
  int get size => value.length;
  
  /// Get number of items (alias for size)
  int get length => value.length;
  
  /// Check if list is empty
  bool get empty => value.isEmpty;
  
  /// Check if list is empty (alias for empty)
  bool get isEmpty => value.isEmpty;
  
  /// Get item at index
  T operator [](int index) => value[index];
  
  /// Add item to list and notify
  void add(T item) {
    final newList = List<T>.from(value);
    newList.add(item);
    value = newList;
    _collectionChangedHandler?.call(StateChangeAction.added, item);
  }
  
  /// Remove item from list and notify
  void remove(T item) {
    final newList = List<T>.from(value);
    if (newList.remove(item)) {
      value = newList;
      _collectionChangedHandler?.call(StateChangeAction.removed, item);
    }
  }
  
  /// Remove items matching predicate
  void removeWhere(bool Function(T) test) {
    final newList = List<T>.from(value);
    final toRemove = newList.where(test).toList();
    for (final item in toRemove) {
      newList.remove(item);
      _collectionChangedHandler?.call(StateChangeAction.removed, item);
    }
    if (toRemove.isNotEmpty) {
      value = newList;
    }
  }
  
  /// Clear all items
  void clear() {
    final itemsCopy = List<T>.from(value);
    value = [];
    for (final item in itemsCopy) {
      _collectionChangedHandler?.call(StateChangeAction.removed, item);
    }
  }
  
  /// Convert to regular list
  List<T> toList() => List<T>.from(value);
  
  /// Set collection changed handler (callback)
  void setCollectionChangedHandler(void Function(StateChangeAction action, T item) handler) {
    _collectionChangedHandler = handler;
  }
  
  /// Iterator support
  Iterator<T> get iterator => value.iterator;
  
  /// For-in loop support  
  Iterable<T> get iterable => value;
}