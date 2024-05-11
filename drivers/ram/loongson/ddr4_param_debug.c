#include <stddef.h>
#include "mem_ctrl.h"

//param_array node0_mc0_param[] =
//{
////{offset, param_value},
////{0x1730, 0x0000000000000000},	example
//{0x0000, 0x0000000000000000}	//termination
//};
//
//param_array node0_mc1_param[] =
//{
//{0x0000,0x0000000000000000}	//termination
//};
//
//param_array node0_mc2_param[] =
//{
//{0x0000,0x0000000000000000}	//termination
//};
//
//param_array node0_mc3_param[] =
//{
//{0x0000,0x0000000000000000}	//termination
//};
//
//param_array node1_mc0_param[] =
//{
//{0x0000,0x0000000000000000}	//termination
//};
//
//param_array node1_mc1_param[] =
//{
//{0x0000,0x0000000000000000}	//termination
//};
//
//param_array node1_mc2_param[] =
//{
//{0x0000,0x0000000000000000}	//termination
//};
//
//param_array node1_mc3_param[] =
//{
//{0x0000,0x0000000000000000}	//termination
//};
//
//param_array node2_mc0_param[] =
//{
//{0x0000,0x0000000000000000}	//termination
//};
//
//param_array node2_mc1_param[] =
//{
//{0x0000,0x0000000000000000}	//termination
//};
//
//param_array node2_mc2_param[] =
//{
//{0x0000,0x0000000000000000}	//termination
//};
//
//param_array node2_mc3_param[] =
//{
//{0x0000,0x0000000000000000}	//termination
//};
//
//param_array node2_mc0_param[] =
//{
//{0x0000,0x0000000000000000}	//termination
//};
//
//param_array node2_mc1_param[] =
//{
//{0x0000,0x0000000000000000}	//termination
//};
//
//param_array node2_mc2_param[] =
//{
//{0x0000,0x0000000000000000}	//termination
//};
//
//param_array node2_mc3_param[] =
//{
//{0x0000,0x0000000000000000}	//termination
//};
//
//param_array node3_mc0_param[] =
//{
//{0x0000,0x0000000000000000}	//termination
//};
//
//param_array node3_mc1_param[] =
//{
//{0x0000,0x0000000000000000}	//termination
//};
//
//param_array node3_mc2_param[] =
//{
//{0x0000,0x0000000000000000}	//termination
//};
//
//param_array node3_mc3_param[] =
//{
//{0x0000,0x0000000000000000}	//termination
//};
//
//param_array node4_mc0_param[] =
//{
//{0x0000,0x0000000000000000}	//termination
//};
//
//param_array node4_mc1_param[] =
//{
//{0x0000,0x0000000000000000}	//termination
//};
//
//param_array node4_mc2_param[] =
//{
//{0x0000,0x0000000000000000}	//termination
//};
//
//param_array node4_mc3_param[] =
//{
//{0x0000,0x0000000000000000}	//termination
//};
//
//param_array node5_mc0_param[] =
//{
//{0x0000,0x0000000000000000}	//termination
//};
//
//param_array node5_mc1_param[] =
//{
//{0x0000,0x0000000000000000}	//termination
//};
//
//param_array node5_mc2_param[] =
//{
//{0x0000,0x0000000000000000}	//termination
//};
//
//param_array node5_mc3_param[] =
//{
//{0x0000,0x0000000000000000}	//termination
//};
//
//param_array node6_mc0_param[] =
//{
//{0x0000,0x0000000000000000}	//termination
//};
//
//param_array node6_mc1_param[] =
//{
//{0x0000,0x0000000000000000}	//termination
//};
//
//param_array node6_mc2_param[] =
//{
//{0x0000,0x0000000000000000}	//termination
//};
//
//param_array node6_mc3_param[] =
//{
//{0x0000,0x0000000000000000}	//termination
//};
//
//param_array node7_mc0_param[] =
//{
//{0x0000,0x0000000000000000}	//termination
//};
//
//param_array node7_mc1_param[] =
//{
//{0x0000,0x0000000000000000}	//termination
//};
//
//param_array node7_mc2_param[] =
//{
//{0x0000,0x0000000000000000}	//termination
//};
//
//param_array node7_mc3_param[] =
//{
//{0x0000,0x0000000000000000}	//termination
//};
//
//param_array node8_mc0_param[] =
//{
//{0x0000,0x0000000000000000}	//termination
//};
//
//param_array node8_mc1_param[] =
//{
//{0x0000,0x0000000000000000}	//termination
//};
//
//param_array node8_mc2_param[] =
//{
//{0x0000,0x0000000000000000}	//termination
//};
//
//param_array node8_mc3_param[] =
//{
//{0x0000,0x0000000000000000}	//termination
//};
//
//param_array node9_mc0_param[] =
//{
//{0x0000,0x0000000000000000}	//termination
//};
//
//param_array node9_mc1_param[] =
//{
//{0x0000,0x0000000000000000}	//termination
//};
//
//param_array node9_mc2_param[] =
//{
//{0x0000,0x0000000000000000}	//termination
//};
//
//param_array node9_mc3_param[] =
//{
//{0x0000,0x0000000000000000}	//termination
//};
//
//param_array node10_mc0_param[] =
//{
//{0x0000,0x0000000000000000}	//termination
//};
//
//param_array node10_mc1_param[] =
//{
//{0x0000,0x0000000000000000}	//termination
//};
//
//param_array node10_mc2_param[] =
//{
//{0x0000,0x0000000000000000}	//termination
//};
//
//param_array node10_mc3_param[] =
//{
//{0x0000,0x0000000000000000}	//termination
//};
//
//param_array node11_mc0_param[] =
//{
//{0x0000,0x0000000000000000}	//termination
//};
//
//param_array node11_mc1_param[] =
//{
//{0x0000,0x0000000000000000}	//termination
//};
//
//param_array node11_mc2_param[] =
//{
//{0x0000,0x0000000000000000}	//termination
//};
//
//param_array node11_mc3_param[] =
//{
//{0x0000,0x0000000000000000}	//termination
//};
//
//param_array node12_mc0_param[] =
//{
//{0x0000,0x0000000000000000}	//termination
//};
//
//param_array node12_mc1_param[] =
//{
//{0x0000,0x0000000000000000}	//termination
//};
//
//param_array node12_mc2_param[] =
//{
//{0x0000,0x0000000000000000}	//termination
//};
//
//param_array node12_mc3_param[] =
//{
//{0x0000,0x0000000000000000}	//termination
//};
//
//param_array node13_mc0_param[] =
//{
//{0x0000,0x0000000000000000}	//termination
//};
//
//param_array node13_mc1_param[] =
//{
//{0x0000,0x0000000000000000}	//termination
//};
//
//param_array node13_mc2_param[] =
//{
//{0x0000,0x0000000000000000}	//termination
//};
//
//param_array node13_mc3_param[] =
//{
//{0x0000,0x0000000000000000}	//termination
//};
//
//param_array node14_mc0_param[] =
//{
//{0x0000,0x0000000000000000}	//termination
//};
//
//param_array node14_mc1_param[] =
//{
//{0x0000,0x0000000000000000}	//termination
//};
//
//param_array node14_mc2_param[] =
//{
//{0x0000,0x0000000000000000}	//termination
//};
//
//param_array node14_mc3_param[] =
//{
//{0x0000,0x0000000000000000}	//termination
//};
//
//param_array node15_mc0_param[] =
//{
//{0x0000,0x0000000000000000}	//termination
//};
//
//param_array node15_mc1_param[] =
//{
//{0x0000,0x0000000000000000}	//termination
//};
//
//param_array node15_mc2_param[] =
//{
//{0x0000,0x0000000000000000}	//termination
//};
//
//param_array node15_mc3_param[] =
//{
//{0x0000,0x0000000000000000}	//termination
//};

//param_array ls7a_mc0_param[] =
//{
//{0x0000,0x0000000000000000}	//termination
//};

param_debug param_info[] =
{
//{node_id, mc_id, *param_array},
//{0,    0,   &node0_mc0_param},	example
{0xff, 0xf, NULL} //do not delete
};
param_debug param_info_ls7a[] =
{
//{0,    0,   &ls7a_mc0_param},	example
{0xff, 0xf, NULL} //do not delete
};
