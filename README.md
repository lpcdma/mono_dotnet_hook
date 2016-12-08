# mono_dotnet_hook
dotnet hook in mono

加载dll并执行入口函数：

![image](https://github.com/evilzhou/mono_dotnet_hook/blob/master/docs/1.png)


DLL实现示例：

![image](https://github.com/evilzhou/mono_dotnet_hook/blob/master/docs/2.png)

![image](https://github.com/evilzhou/mono_dotnet_hook/blob/master/docs/3.png)


执行日志：

![image](https://github.com/evilzhou/mono_dotnet_hook/blob/master/docs/4.png)


注意：

  1、加载dll时报root image null错误，增加延迟加载时间即可
  
  2、加载dll时报map file failed错误，可能是dll权限不够，777即可
  
  3、DLL中CSharpHook、CSharpUnhook、Logd函数签名固定，位于加载参数的Class中即可

  4、若原函数为类中类函数，类之间用/区分，如：Assembly-CSharp..RDARScript/Parser.importScript

  5、重载函数使用参数签名区分，如：Assembly-CSharp..NpcScript/Parser.importScript(string)

  6、模版类，如class FixQueue<T>，使用：FixQueue`1
