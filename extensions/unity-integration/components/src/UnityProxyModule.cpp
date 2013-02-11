#include "nsIGenericFactory.h"
#include "UnityProxy.h"

// Создаём конструктор (фабрику) нашего компонента. Этот макрос
// создаёт static функцию с именем <имя_аргумента>Constructor, в нашем случае
// UnityProxyConstructor, которая далее используется в структуре components.
NS_GENERIC_FACTORY_CONSTRUCTOR(UnityProxy)

// Структура с описанием нашего компонента
static nsModuleComponentInfo components[] =
{
    {
       UNITY_PROXY_CLASSNAME,
       UNITY_PROXY_CID,
       UNITY_PROXY_CONTRACTID,
       UnityProxyConstructor,
    }
};

// Создаём точку входа с необходимой информацией. Это аналогично extern "C" функциям
// в С++ библиотеках, загружаемых через dlopen(), и которые создают и возвращают указатель
// на объект класса.
NS_IMPL_NSGETMODULE("UnityProxysModule", components)
