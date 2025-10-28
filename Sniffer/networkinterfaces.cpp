#include "networkinterfaces.h"

void print_network_interfaces() {
    PIP_ADAPTER_INFO adapter_info;
    PIP_ADAPTER_INFO adapter = NULL;
    DWORD dw_ret_val = 0;
    ULONG ul_out_buf_len = sizeof(IP_ADAPTER_INFO);

    adapter_info = (IP_ADAPTER_INFO*)malloc(ul_out_buf_len);

    if (GetAdaptersInfo(adapter_info, &ul_out_buf_len) == ERROR_BUFFER_OVERFLOW) {
        free(adapter_info);
        adapter_info = (IP_ADAPTER_INFO*)malloc(ul_out_buf_len);
    }

    if ((dw_ret_val = GetAdaptersInfo(adapter_info, &ul_out_buf_len)) == NO_ERROR) {
        adapter = adapter_info;
        while (adapter) {
            std::cout << "Adapter: " << adapter->Description << std::endl;
            std::cout << "  IP Address: " << adapter->IpAddressList.IpAddress.String << std::endl;
            std::cout << "  Subnet Mask: " << adapter->IpAddressList.IpMask.String << std::endl;
            std::cout << "  Gateway: " << adapter->GatewayList.IpAddress.String << std::endl;
            std::cout << "----------------------------------------" << std::endl;
            adapter = adapter->Next;
        }
    }
    free(adapter_info);
}
