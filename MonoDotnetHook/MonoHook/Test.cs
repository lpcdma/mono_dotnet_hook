using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Runtime.CompilerServices;
using UnityEngine;

namespace MonoHook
{
    public class Test
    {
        [MethodImpl(MethodImplOptions.InternalCall)]

        public static extern void Logd(string log);

        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern void CSharpHook(string target, string replace, string old);

        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern void CSharpUnhook(string target);

        public static void Entry()
        {
            Logd("enter plugin...");

            var obj = new GameObject("TestBehavior", typeof(TestBehavior));
            UnityEngine.Object.DontDestroyOnLoad(obj);

            Logd("exit plugin...");
        }
    }
}
