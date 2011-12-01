/* ----------------------------------------------------------------------------
 * This file was automatically generated by SWIG (http://www.swig.org).
 * Version 1.3.35
 *
 * Do not make changes to this file unless you know what you are doing--modify
 * the SWIG interface file instead.
 * ----------------------------------------------------------------------------- */

package be.belgium.eid;

/******************************************************************************//**
  * Class for the id document on a SIS Card.
  * To get this object: BEID_SISCard.getID() BEID_SISCard.getDocument()
  *********************************************************************************/
public class BEID_SisId extends BEID_XMLDoc {
  private long swigCPtr;

  protected BEID_SisId(long cPtr, boolean cMemoryOwn) {
    super(beidlibJava_WrapperJNI.SWIGBEID_SisIdUpcast(cPtr), cMemoryOwn);
    swigCPtr = cPtr;
  }

  protected static long getCPtr(BEID_SisId obj) {
    return (obj == null) ? 0 : obj.swigCPtr;
  }

  protected void finalize() {
    delete();
  }

  public synchronized void delete() {
    if(swigCPtr != 0 && swigCMemOwn) {
      swigCMemOwn = false;
      beidlibJava_WrapperJNI.delete_BEID_SisId(swigCPtr);
    }
    swigCPtr = 0;
    super.delete();
  }

	/** Get the name field */
  public String getName() throws java.lang.Exception {
    return beidlibJava_WrapperJNI.BEID_SisId_getName(swigCPtr, this);
  }

	/** Get the surname field */
  public String getSurname() throws java.lang.Exception {
    return beidlibJava_WrapperJNI.BEID_SisId_getSurname(swigCPtr, this);
  }

	/** Get the initials field */
  public String getInitials() throws java.lang.Exception {
    return beidlibJava_WrapperJNI.BEID_SisId_getInitials(swigCPtr, this);
  }

	/** Get the Gender field */
  public String getGender() throws java.lang.Exception {
    return beidlibJava_WrapperJNI.BEID_SisId_getGender(swigCPtr, this);
  }

	/** Get the Date Of Birth field */
  public String getDateOfBirth() throws java.lang.Exception {
    return beidlibJava_WrapperJNI.BEID_SisId_getDateOfBirth(swigCPtr, this);
  }

	/** Get the Social Security Number field */
  public String getSocialSecurityNumber() throws java.lang.Exception {
    return beidlibJava_WrapperJNI.BEID_SisId_getSocialSecurityNumber(swigCPtr, this);
  }

	/** Get the Logical Number field */
  public String getLogicalNumber() throws java.lang.Exception {
    return beidlibJava_WrapperJNI.BEID_SisId_getLogicalNumber(swigCPtr, this);
  }

	/** Get the Date Of Issue field */
  public String getDateOfIssue() throws java.lang.Exception {
    return beidlibJava_WrapperJNI.BEID_SisId_getDateOfIssue(swigCPtr, this);
  }

	/** Get the Validity Begin Date field */
  public String getValidityBeginDate() throws java.lang.Exception {
    return beidlibJava_WrapperJNI.BEID_SisId_getValidityBeginDate(swigCPtr, this);
  }

	/** Get the Validity End Date field */
  public String getValidityEndDate() throws java.lang.Exception {
    return beidlibJava_WrapperJNI.BEID_SisId_getValidityEndDate(swigCPtr, this);
  }

}