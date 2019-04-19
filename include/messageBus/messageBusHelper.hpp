#pragma once

#include <string>
const std::string MSG_BUS_NAME = "MSG_BUS_NAME";
template <typename BUS_MSG_TYPE>
class messageBusHelper : public gene::gene
{
public:
    messageBusHelper() = delete;
    messageBusHelper(void *gene)
    {
        set_genetic_gene(gene);
    }
    template <typename BUS_MSG_TYPE>
    std::shared_ptr<messageBus> getMessageBus()
    {
        anySaver<BUS_MSG_TYPE>::instance()->getData(get_genetic_gene(), MSG_BUS_NAME);
    }

    template <typename BUS_MSG_TYPE>
    messageBus setMessageBus()
    {
        anySaver<BUS_MSG_TYPE>::instance()->saveData(get_genetic_gene(), MSG_BUS_NAME, std::make_shred<messageBus>());
    }
}