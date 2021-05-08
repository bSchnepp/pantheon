#include <kern_integers.hpp>
#include <kern_datatypes.hpp>

#ifndef _PCIE_HPP_
#define _PCIE_HPP_

class PCIe
{
public:
	PCIe(void *Address);
	~PCIe();

private:
	void *Address;
};


#endif