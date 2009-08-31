/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2009 Joshua Haberman.  See LICENSE for details.
 */

#include <inttypes.h>
#include "descriptor.h"
#include "upb_text.h"
#include "upb_string.h"
#include "upb_msg.h"
#include "upb_array.h"

void upb_text_printval(upb_field_type_t type, union upb_value val, FILE *file)
{
#define CASE(fmtstr, member) fprintf(file, fmtstr, val.member); break;
  switch(type) {
    case GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_DOUBLE:
      CASE("%0.f", _double);
    case GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_FLOAT:
      CASE("%0.f", _float)
    case GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_INT64:
    case GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_SFIXED64:
    case GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_SINT64:
      CASE("%" PRId64, int64)
    case GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_UINT64:
    case GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_FIXED64:
      CASE("%" PRIu64, uint64)
    case GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_INT32:
    case GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_SFIXED32:
    case GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_SINT32:
      CASE("%" PRId32, int32)
    case GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_UINT32:
    case GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_FIXED32:
    case GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_ENUM:
      CASE("%" PRIu32, uint32);
    case GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_BOOL:
      CASE("%hhu", _bool);
    case GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_STRING:
    case GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_BYTES:
      /* TODO: escaping. */
      fprintf(file, "\"" UPB_STRFMT "\"", UPB_STRARG(val.str)); break;
  }
}

static void print_indent(struct upb_text_printer *p, FILE *stream)
{
  if(!p->single_line)
    for(int i = 0; i < p->indent_depth; i++)
      fprintf(stream, "  ");
}

void upb_text_printfield(struct upb_text_printer *p,
                         struct upb_string *name,
                         upb_field_type_t valtype, union upb_value val,
                         FILE *stream)
{
  print_indent(p, stream);
  fprintf(stream, UPB_STRFMT ":", UPB_STRARG(name));
  upb_text_printval(valtype, val, stream);
  if(p->single_line)
    fputc(' ', stream);
  else
    fputc('\n', stream);
}

void upb_text_push(struct upb_text_printer *p,
                   struct upb_string *submsg_type,
                   FILE *stream)
{
  print_indent(p, stream);
  fprintf(stream, UPB_STRFMT " {", UPB_STRARG(submsg_type));
  if(!p->single_line) fputc('\n', stream);
  p->indent_depth++;
}

void upb_text_pop(struct upb_text_printer *p,
                  FILE *stream)
{
  p->indent_depth--;
  print_indent(p, stream);
  fprintf(stream, "}\n");
}

static void printval(struct upb_text_printer *printer, union upb_value_ptr p,
                     struct upb_msg_fielddef *f,
                     google_protobuf_FieldDescriptorProto *fd,
                     FILE *stream);

static void printmsg(struct upb_text_printer *printer, struct upb_msg *msg,
                     FILE *stream)
{
  struct upb_msgdef *m = msg->def;
  for(uint32_t i = 0; i < m->num_fields; i++) {
    struct upb_msg_fielddef *f = &m->fields[i];
    google_protobuf_FieldDescriptorProto *fd = upb_msg_field_descriptor(f, m);
    if(!upb_msg_isset(msg, f)) continue;
    union upb_value_ptr p = upb_msg_getptr(msg, f);
    if(upb_isarray(f)) {
      struct upb_array *arr = *p.arr;
      for(uint32_t j = 0; j < arr->len; j++) {
        union upb_value_ptr elem_p = upb_array_getelementptr(arr, j);
        printval(printer, elem_p, f, fd, stream);
      }
    } else {
      printval(printer, p, f, fd, stream);
    }
  }
}

static void printval(struct upb_text_printer *printer, union upb_value_ptr p,
                     struct upb_msg_fielddef *f,
                     google_protobuf_FieldDescriptorProto *fd,
                     FILE *stream)
{
  if(upb_issubmsg(f)) {
    upb_text_push(printer, fd->name, stream);
    printmsg(printer, *p.msg, stream);
    upb_text_pop(printer, stream);
  } else {
    upb_text_printfield(printer, fd->name, f->type, upb_value_read(p, f->type), stream);
  }
}


void upb_msg_print(struct upb_msg *msg, bool single_line, FILE *stream)
{
  struct upb_text_printer printer;
  upb_text_printer_init(&printer, single_line);
  printmsg(&printer, msg, stream);
}
