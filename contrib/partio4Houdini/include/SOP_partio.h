/* partio4Houdini  5/01/2013, Miles Green

 PARTIO Import/Export
 Copyright 2013 (c)  All rights reserved

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are
 met:

 * Redistributions of source code must retain the above copyright
 notice, this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright
 notice, this list of conditions and the following disclaimer in
 the documentation and/or other materials provided with the
 distribution.

 Disclaimer: THIS SOFTWARE IS PROVIDED BY  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING,
 BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
 FOR A PARTICULAR PURPOSE, NONINFRINGEMENT AND TITLE ARE DISCLAIMED.
 IN NO EVENT SHALL  THE COPYRIGHT HOLDER OR
 CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND BASED ON ANY
 THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.
 */


// Notes: this is the header for the SOP partio node

#ifndef __SOP_partio_h__
#define __SOP_partio_h__

#include <SOP/SOP_Node.h>

#define STR_PARM(name, vi, t) \
		{ evalString(str, name, vi, t); }
#define INT_PARM(name, vi, t) \
                { return evalInt(name, vi, t); }

namespace partio {
class SOP_partio : public SOP_Node
{
public:
    static OP_Node	*myConstructor(OP_Network*, const char *, OP_Operator *);

    /// Stores the description of the interface of the SOP in Houdini.
    /// Each parm template refers to a parameter.
    static PRM_Template		 myTemplateList[];

    /// This optional data stores the list of local variables.
    static CH_LocalVariable	 myVariables[];

    virtual void         opChanged(OP_EventType reason, void *data=0);


protected:
    SOP_partio(OP_Network *net, const char *name, OP_Operator *op);
    virtual ~SOP_partio();

    /// Disable parameters according to other parameters.
    //virtual unsigned		 disableParms();

    static int refreshStatic(void *op, int, fpreal time, const PRM_Template *);
    void refresh();

    /// cookMySop does the actual work of the SOP computing, in this
    /// case, a star shape.
    virtual OP_ERROR		 cookMySop(OP_Context &context);

    //override help
    virtual bool getHDKHelp(UT_String &str)const ;

    /// This function is used to lookup local variables that you have
    /// defined specific to your SOP.
    //virtual float		 getVariableValue(int index, int thread);
    //virtual void		 getVariableString(int index,UT_String &value, int thread);

    /*virtual bool                 evalVariableValue(fpreal &val,int index,int thread);*/
    // Add virtual overload that delegates to the super class to avoid
    // shadow warnings.
    /*virtual bool                 evalVariableValue( UT_String &v,int i,int thread);*/

    virtual bool shouldResetGeoToEmpty() const
    {
        return false;
    }

private:
    /// The following list of accessors simplify evaluating the parameters
    /// of the SOP.

    int	  VERBOSITY(float t)
	{
        return evalInt  ("dbug", 0, t);
    }
    int	  IOMODE(float t)
	{
        INT_PARM("filemode", 0, t)
    }
    void  FILEPATH(UT_String &str, float t)
	{
        STR_PARM("file",  0, t)
    }
    bool  matchExtension(UT_String &str);

    /// Member variables are stored in the actual SOP, not with the geometry
    /// In this case these are just used to transfer data to the local
    /// variable callback.
    /// Another use for local data is a cache to store expensive calculations.

    float       myCurrentFrame;
    UT_String   myCurrentFileName;

};
} // End HDK_Sample namespace

#endif
