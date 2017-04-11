using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Runtime.CompilerServices;
using UnityEngine;

namespace MonoHook
{
    class TestBehavior : MonoBehaviour
    {
        [MethodImpl(MethodImplOptions.InternalCall)]

        public static extern void oldMove(float move, bool crouch, bool jump);

        public static void MyMove(float move, bool crouch, bool jump)
        {
            Test.Logd(String.Format("XMONO ====== lpcdma %f", move));
            oldMove(move, crouch, jump);
        }

        void OnGUI()
        {
            if (GUI.Button(new Rect(Screen.width*5/12, Screen.height/4, 200, 100), "hook"))
            {
                Test.CSharpHook("Assembly-CSharp-firstpass..PlatformerCharacter2D.Move", "MonoHook.MonoHook.TestBehavior.MyMove", "MonoHook.MonoHook.TestBehavior.oldMove");
            }
            if (GUI.Button(new Rect(Screen.width * 6 / 12, Screen.height / 4, 200, 100), "unhook"))
            {
                Test.CSharpUnhook("Assembly-CSharp-firstpass..PlatformerCharacter2D.Move");
            }
        }

        void Start()
        {
            Test.Logd("test behavior ....");
        }
    }
}
