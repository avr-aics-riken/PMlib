//
// List of the names defined as power objects.
// Current implementation is limited to supercomputer Fugaku
// Power objects and attributes are grouped into default context and extended context
// Note that some of them reside on both groups
//

// index of Objects supported by default context
enum power_object_index
	{
		I_pobj_NODE=0,
		I_pobj_CMG0CORES,
		I_pobj_CMG1CORES,
		I_pobj_CMG2CORES,
		I_pobj_CMG3CORES,
		I_pobj_CMG0L2CACHE,
		I_pobj_CMG1L2CACHE,
		I_pobj_CMG2L2CACHE,
		I_pobj_CMG3L2CACHE,
		I_pobj_ACORES0,
		I_pobj_ACORES1,
		I_pobj_TOFU,
		I_pobj_UNCMG,
		I_pobj_MEM0,
		I_pobj_MEM1,
		I_pobj_MEM2,
		I_pobj_MEM3,
		I_pobj_PCI,
		I_pobj_TOFUOPT,
		Max_power_object		// =19
	};

// Object string supported by default context
static char p_obj_name[Max_power_object][30] =
	{
		"plat.node",
		"plat.node.cpu.cmg0.cores",
		"plat.node.cpu.cmg1.cores",
		"plat.node.cpu.cmg2.cores",
		"plat.node.cpu.cmg3.cores",
		"plat.node.cpu.cmg0.l2cache",
		"plat.node.cpu.cmg1.l2cache",
		"plat.node.cpu.cmg2.l2cache",
		"plat.node.cpu.cmg3.l2cache",
		"plat.node.cpu.acores.core0",
		"plat.node.cpu.acores.core1",
		"plat.node.cpu.tofu",
		"plat.node.cpu.uncmg",
		"plat.node.mem0",
		"plat.node.mem1",
		"plat.node.mem2",
		"plat.node.mem3",
		"plat.node.pci",
		"plat.node.tofuopt"
	};

// index of Objects supported by extended context
// these are used
// (a) to obtain the actually measured value from sensor device
//     currently, the only supported object is "NODE:I_pext_NODE" on Fugaku.
// (b) to enable power knob controll
//     extended parts including CPU/COMnCORES/MEMn are defined to enable power knob controll
enum power_extended_index
	{
		I_pext_NODE=0,
		I_pext_CPU,
		I_pext_CMG0CORES,
		I_pext_CMG1CORES,
		I_pext_CMG2CORES,
		I_pext_CMG3CORES,
		I_pext_MEM0,
		I_pext_MEM1,
		I_pext_MEM2,
		I_pext_MEM3,
		Max_power_extended
	};

// Object string supported by extended context
static char p_ext_name[Max_power_extended][30] =
	{
		"plat.node",
		"plat.node.cpu",	// valid both for default and exxtened context
		"plat.node.cpu.cmg0.cores",	//	ditto.
		"plat.node.cpu.cmg1.cores",	//	ditto.
		"plat.node.cpu.cmg2.cores",	//	ditto.
		"plat.node.cpu.cmg3.cores",	//	ditto.
		"plat.node.mem0",	//  can set the value for the estimation
		"plat.node.mem1",	//	ditto.
		"plat.node.mem2",	//	ditto.
		"plat.node.mem3"	//	ditto.
	};

// index of power knob types that can be controlled by users
enum power_knob_index
	{
		I_knob_CPU=0,
		I_knob_MEMORY,
		I_knob_ISSUE,
		I_knob_PIPE,
		I_knob_ECO,
		//	I_knob_RETENTION,	// disabled
		Max_power_knob
	};

const int Max_measure_device=1;
	// NODE ("plat.node") is the only attribute to get the power meter measured value from
const int Max_power_leaf_parts=12;
	// max. # of leaf parts in the object group, i.e. 12 cores in the CMG.


