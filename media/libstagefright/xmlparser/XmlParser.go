package xmlparser

import (
    "android/soong/android"
    "android/soong/cc"
    "fmt"
)

func xmlparserDefaults (ctx android.LoadHookContext) {
    sdkVersion := ctx.AConfig().PlatformSdkVersionInt()
    fmt.Println("sdkVersion:", sdkVersion)
    if sdkVersion >= 30{
        type props struct {
        Cflags []string
        }
        p := &props{}
        p.Cflags = globalDefaults(ctx)
        ctx.AppendProperties(p)
    }
}

func globalDefaults(ctx android.BaseContext) ([]string) {
    var cppflags []string
    vconfig := ctx.Config().VendorConfig("amlogic_vendorconfig")
    if vconfig.Bool("enable_swcodec") == true {
        fmt.Println("stagefright: enable_swcodec is true")
        cppflags = append(cppflags,"-DTARGETSWCODEC_EXT")
    }
    return cppflags
}

func init () {
    android.RegisterModuleType("xmlparser_defaults", xmlparserDefaultsFactory)
}

func xmlparserDefaultsFactory () android.Module {
    module := cc.DefaultsFactory()
    android.AddLoadHook(module, xmlparserDefaults)
    return module
}

