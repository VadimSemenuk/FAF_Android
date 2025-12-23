package com.pragmatsoft.faf.data.interfaces

interface StringProvider {
    fun getString(resourceId: Int, vararg formatArgs: Any): String
    fun getQuantityString(resourceId: Int, quality: Int, vararg formatArgs: Any): String
}