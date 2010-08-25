/**
 * Autogenerated by Thrift
 *
 * DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
 */
package org.hypertable.thriftgen;

import java.util.List;
import java.util.ArrayList;
import java.util.Map;
import java.util.HashMap;
import java.util.EnumMap;
import java.util.Set;
import java.util.HashSet;
import java.util.EnumSet;
import java.util.Collections;
import java.util.BitSet;
import java.nio.ByteBuffer;
import java.util.Arrays;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import org.apache.thrift.*;
import org.apache.thrift.async.*;
import org.apache.thrift.meta_data.*;
import org.apache.thrift.transport.*;
import org.apache.thrift.protocol.*;

/**
 * Defines an individual namespace listing
 * 
 * <dl>
 *   <dt>name</dt>
 *   <dd>Name of the listing.</dd>
 * 
 *   <dt>is_namespace</dt>
 *   <dd>true if this entry is a namespace.</dd>
 * </dl>
 */
public class NamespaceListing implements TBase<NamespaceListing, NamespaceListing._Fields>, java.io.Serializable, Cloneable {
  private static final TStruct STRUCT_DESC = new TStruct("NamespaceListing");

  private static final TField NAME_FIELD_DESC = new TField("name", TType.STRING, (short)1);
  private static final TField IS_NAMESPACE_FIELD_DESC = new TField("is_namespace", TType.BOOL, (short)2);

  public String name;
  public boolean is_namespace;

  /** The set of fields this struct contains, along with convenience methods for finding and manipulating them. */
  public enum _Fields implements TFieldIdEnum {
    NAME((short)1, "name"),
    IS_NAMESPACE((short)2, "is_namespace");

    private static final Map<String, _Fields> byName = new HashMap<String, _Fields>();

    static {
      for (_Fields field : EnumSet.allOf(_Fields.class)) {
        byName.put(field.getFieldName(), field);
      }
    }

    /**
     * Find the _Fields constant that matches fieldId, or null if its not found.
     */
    public static _Fields findByThriftId(int fieldId) {
      switch(fieldId) {
        case 1: // NAME
          return NAME;
        case 2: // IS_NAMESPACE
          return IS_NAMESPACE;
        default:
          return null;
      }
    }

    /**
     * Find the _Fields constant that matches fieldId, throwing an exception
     * if it is not found.
     */
    public static _Fields findByThriftIdOrThrow(int fieldId) {
      _Fields fields = findByThriftId(fieldId);
      if (fields == null) throw new IllegalArgumentException("Field " + fieldId + " doesn't exist!");
      return fields;
    }

    /**
     * Find the _Fields constant that matches name, or null if its not found.
     */
    public static _Fields findByName(String name) {
      return byName.get(name);
    }

    private final short _thriftId;
    private final String _fieldName;

    _Fields(short thriftId, String fieldName) {
      _thriftId = thriftId;
      _fieldName = fieldName;
    }

    public short getThriftFieldId() {
      return _thriftId;
    }

    public String getFieldName() {
      return _fieldName;
    }
  }

  // isset id assignments
  private static final int __IS_NAMESPACE_ISSET_ID = 0;
  private BitSet __isset_bit_vector = new BitSet(1);

  public static final Map<_Fields, FieldMetaData> metaDataMap;
  static {
    Map<_Fields, FieldMetaData> tmpMap = new EnumMap<_Fields, FieldMetaData>(_Fields.class);
    tmpMap.put(_Fields.NAME, new FieldMetaData("name", TFieldRequirementType.REQUIRED, 
        new FieldValueMetaData(TType.STRING)));
    tmpMap.put(_Fields.IS_NAMESPACE, new FieldMetaData("is_namespace", TFieldRequirementType.REQUIRED, 
        new FieldValueMetaData(TType.BOOL)));
    metaDataMap = Collections.unmodifiableMap(tmpMap);
    FieldMetaData.addStructMetaDataMap(NamespaceListing.class, metaDataMap);
  }

  public NamespaceListing() {
  }

  public NamespaceListing(
    String name,
    boolean is_namespace)
  {
    this();
    this.name = name;
    this.is_namespace = is_namespace;
    setIs_namespaceIsSet(true);
  }

  /**
   * Performs a deep copy on <i>other</i>.
   */
  public NamespaceListing(NamespaceListing other) {
    __isset_bit_vector.clear();
    __isset_bit_vector.or(other.__isset_bit_vector);
    if (other.isSetName()) {
      this.name = other.name;
    }
    this.is_namespace = other.is_namespace;
  }

  public NamespaceListing deepCopy() {
    return new NamespaceListing(this);
  }

  @Deprecated
  public NamespaceListing clone() {
    return new NamespaceListing(this);
  }

  @Override
  public void clear() {
    this.name = null;
    setIs_namespaceIsSet(false);
    this.is_namespace = false;
  }

  public String getName() {
    return this.name;
  }

  public NamespaceListing setName(String name) {
    this.name = name;
    return this;
  }

  public void unsetName() {
    this.name = null;
  }

  /** Returns true if field name is set (has been asigned a value) and false otherwise */
  public boolean isSetName() {
    return this.name != null;
  }

  public void setNameIsSet(boolean value) {
    if (!value) {
      this.name = null;
    }
  }

  public boolean isIs_namespace() {
    return this.is_namespace;
  }

  public NamespaceListing setIs_namespace(boolean is_namespace) {
    this.is_namespace = is_namespace;
    setIs_namespaceIsSet(true);
    return this;
  }

  public void unsetIs_namespace() {
    __isset_bit_vector.clear(__IS_NAMESPACE_ISSET_ID);
  }

  /** Returns true if field is_namespace is set (has been asigned a value) and false otherwise */
  public boolean isSetIs_namespace() {
    return __isset_bit_vector.get(__IS_NAMESPACE_ISSET_ID);
  }

  public void setIs_namespaceIsSet(boolean value) {
    __isset_bit_vector.set(__IS_NAMESPACE_ISSET_ID, value);
  }

  public void setFieldValue(_Fields field, Object value) {
    switch (field) {
    case NAME:
      if (value == null) {
        unsetName();
      } else {
        setName((String)value);
      }
      break;

    case IS_NAMESPACE:
      if (value == null) {
        unsetIs_namespace();
      } else {
        setIs_namespace((Boolean)value);
      }
      break;

    }
  }

  public void setFieldValue(int fieldID, Object value) {
    setFieldValue(_Fields.findByThriftIdOrThrow(fieldID), value);
  }

  public Object getFieldValue(_Fields field) {
    switch (field) {
    case NAME:
      return getName();

    case IS_NAMESPACE:
      return new Boolean(isIs_namespace());

    }
    throw new IllegalStateException();
  }

  public Object getFieldValue(int fieldId) {
    return getFieldValue(_Fields.findByThriftIdOrThrow(fieldId));
  }

  /** Returns true if field corresponding to fieldID is set (has been asigned a value) and false otherwise */
  public boolean isSet(_Fields field) {
    switch (field) {
    case NAME:
      return isSetName();
    case IS_NAMESPACE:
      return isSetIs_namespace();
    }
    throw new IllegalStateException();
  }

  public boolean isSet(int fieldID) {
    return isSet(_Fields.findByThriftIdOrThrow(fieldID));
  }

  @Override
  public boolean equals(Object that) {
    if (that == null)
      return false;
    if (that instanceof NamespaceListing)
      return this.equals((NamespaceListing)that);
    return false;
  }

  public boolean equals(NamespaceListing that) {
    if (that == null)
      return false;

    boolean this_present_name = true && this.isSetName();
    boolean that_present_name = true && that.isSetName();
    if (this_present_name || that_present_name) {
      if (!(this_present_name && that_present_name))
        return false;
      if (!this.name.equals(that.name))
        return false;
    }

    boolean this_present_is_namespace = true;
    boolean that_present_is_namespace = true;
    if (this_present_is_namespace || that_present_is_namespace) {
      if (!(this_present_is_namespace && that_present_is_namespace))
        return false;
      if (this.is_namespace != that.is_namespace)
        return false;
    }

    return true;
  }

  @Override
  public int hashCode() {
    return 0;
  }

  public int compareTo(NamespaceListing other) {
    if (!getClass().equals(other.getClass())) {
      return getClass().getName().compareTo(other.getClass().getName());
    }

    int lastComparison = 0;
    NamespaceListing typedOther = (NamespaceListing)other;

    lastComparison = Boolean.valueOf(isSetName()).compareTo(typedOther.isSetName());
    if (lastComparison != 0) {
      return lastComparison;
    }
    if (isSetName()) {      lastComparison = TBaseHelper.compareTo(this.name, typedOther.name);
      if (lastComparison != 0) {
        return lastComparison;
      }
    }
    lastComparison = Boolean.valueOf(isSetIs_namespace()).compareTo(typedOther.isSetIs_namespace());
    if (lastComparison != 0) {
      return lastComparison;
    }
    if (isSetIs_namespace()) {      lastComparison = TBaseHelper.compareTo(this.is_namespace, typedOther.is_namespace);
      if (lastComparison != 0) {
        return lastComparison;
      }
    }
    return 0;
  }

  public void read(TProtocol iprot) throws TException {
    TField field;
    iprot.readStructBegin();
    while (true)
    {
      field = iprot.readFieldBegin();
      if (field.type == TType.STOP) { 
        break;
      }
      switch (field.id) {
        case 1: // NAME
          if (field.type == TType.STRING) {
            this.name = iprot.readString();
          } else { 
            TProtocolUtil.skip(iprot, field.type);
          }
          break;
        case 2: // IS_NAMESPACE
          if (field.type == TType.BOOL) {
            this.is_namespace = iprot.readBool();
            setIs_namespaceIsSet(true);
          } else { 
            TProtocolUtil.skip(iprot, field.type);
          }
          break;
        default:
          TProtocolUtil.skip(iprot, field.type);
      }
      iprot.readFieldEnd();
    }
    iprot.readStructEnd();

    // check for required fields of primitive type, which can't be checked in the validate method
    if (!isSetIs_namespace()) {
      throw new TProtocolException("Required field 'is_namespace' was not found in serialized data! Struct: " + toString());
    }
    validate();
  }

  public void write(TProtocol oprot) throws TException {
    validate();

    oprot.writeStructBegin(STRUCT_DESC);
    if (this.name != null) {
      oprot.writeFieldBegin(NAME_FIELD_DESC);
      oprot.writeString(this.name);
      oprot.writeFieldEnd();
    }
    oprot.writeFieldBegin(IS_NAMESPACE_FIELD_DESC);
    oprot.writeBool(this.is_namespace);
    oprot.writeFieldEnd();
    oprot.writeFieldStop();
    oprot.writeStructEnd();
  }

  @Override
  public String toString() {
    StringBuilder sb = new StringBuilder("NamespaceListing(");
    boolean first = true;

    sb.append("name:");
    if (this.name == null) {
      sb.append("null");
    } else {
      sb.append(this.name);
    }
    first = false;
    if (!first) sb.append(", ");
    sb.append("is_namespace:");
    sb.append(this.is_namespace);
    first = false;
    sb.append(")");
    return sb.toString();
  }

  public void validate() throws TException {
    // check for required fields
    if (name == null) {
      throw new TProtocolException("Required field 'name' was not present! Struct: " + toString());
    }
    // alas, we cannot check 'is_namespace' because it's a primitive and you chose the non-beans generator.
  }

}

