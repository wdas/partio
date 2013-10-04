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


// Notes: This is the header for the ROP partio exporter

#ifndef __ROP_partio_h__
#define __ROP_partio_h__

#include <ROP/ROP_Node.h>


#define STR_PARM(name, vi, t) \
		{ evalString(str, name, vi, t); }
#define STR_PARMRAW(name, vi, t) \
		{ evalStringRaw(str, name, vi, t); }
#define INT_PARM(name, vi, t) \
                { return evalInt(name, vi, t); }
#define FLOAT_PARM(name, vi, t) \
                { return evalFloat(name, vi, t); }




class OP_TemplatePair;
class OP_VariablePair;

namespace HDK_Sample {

enum {
    ROP_PIO_RENDER,
    ROP_PIO_RENDER_CTRL,
    ROP_PIO_TRANGE,
    ROP_PIO_FRANGE,
    ROP_PIO_TAKE,
    ROP_PIO_SOPPATH,
    ROP_PIO_SOPOUTPUT,
    ROP_PIO_VERBOSITY,
    ROP_PIO_INITSIM,
    ROP_PIO_ALFPROGRESS,
    ROP_PIO_TPRERENDER,
    ROP_PIO_PRERENDER,
    ROP_PIO_LPRERENDER,
    ROP_PIO_TPREFRAME,
    ROP_PIO_PREFRAME,
    ROP_PIO_LPREFRAME,
    ROP_PIO_TPOSTFRAME,
    ROP_PIO_POSTFRAME,
    ROP_PIO_LPOSTFRAME,
    ROP_PIO_TPOSTRENDER,
    ROP_PIO_POSTRENDER,
    ROP_PIO_LPOSTRENDER,
    ROP_PIO_MAXPARMS
};
class ROP_partio : public ROP_Node {
public:

    /// Provides access to our parm templates.
    static OP_TemplatePair	*getTemplatePair();
    static OP_VariablePair	*getVariablePair();
    /// Creates an instance of this node.
    static OP_Node		*myConstructor(OP_Network *net, const char*name,
                                   OP_Operator *op);

protected:
    ROP_partio(OP_Network *net, const char *name, OP_Operator *entry);
    virtual ~ROP_partio();

    /// Disable parameters according to other parameters.
    /// virtual unsigned		 disableParms();

    /// Called at the beginning of rendering to perform any intialization
    /// necessary.
    /// @param	nframes	    Number of frames being rendered.
    /// @param	s	    Start time, in seconds.
    /// @param	e	    End time, in seconds.
    /// @return		    True of success, false on failure (aborts the render).
    virtual int			 startRender(int nframes, fpreal s, fpreal e);

    /// Called once for every frame that is rendered.
    /// @param	time	    The time to render at.
    /// @param	boss	    Interrupt handler.
    /// @return		    Return a status code indicating whether to abort the
    ///			    render, continue, or retry the current frame.
    virtual ROP_RENDER_CODE	 renderFrame(fpreal time, UT_Interrupt *boss);

    /// Called after the rendering is done to perform any post-rendering steps
    /// required.
    /// @return		    Return a status code indicating whether to abort the
    ///			    render, continue, or retry.
    virtual ROP_RENDER_CODE	 endRender();

public:

    /// A convenience method to evaluate our custom file parameter.
    void  OUTPUT(UT_String &str, float t)
    {
        STR_PARM("sopoutput",  0, t)
    }
    void  OUTPUTRAW(UT_String &str, float t)
    {
        STR_PARMRAW("sopoutput",  0, t)
    }
    int		INITSIM(void)
    {
        INT_PARM("initsim", 0, 0)
    }
    bool	ALFPROGRESS()
    {
        INT_PARM("alfprogress", 0, 0)
    }
    void	SOPPATH(UT_String &str, float t)
    {
        STR_PARM("soppath", 0, t)
    }


    int	VERBOSITY(void)
    {
        INT_PARM("verbosity", 0, 0)
    }

    int	TRANGE(void)
    {
        INT_PARM("trange", 0, 0)
    }
    float	STARTFRAME(void)
    {
        FLOAT_PARM("f", 0, 0)
    }
    float	ENDFRAME(void)
    {
        FLOAT_PARM("f", 1, 0)
    }





private:
    float		 myEndTime;
    float		 myStartTime;

};

}	// End HDK_Sample namespace


#undef STR_PARM
#undef STR_SET
#undef STR_GET

#endif
