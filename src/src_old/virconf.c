int virConfGetValueOct(virConfPtr conf,
                       const char *setting,
                       unsigned int *value)
{
    virConfValuePtr cval = virConfGetValue(conf, setting);

    VIR_DEBUG("Get value octal %p %d",
              cval, cval ? cval->type : VIR_CONF_NONE);

    if (!cval)
        return 0;

    if (cval->type != VIR_CONF_ULLONG) {
        virReportError(VIR_ERR_INTERNAL_ERROR,
                       _("%: expected an unsigned"))
    }
}
